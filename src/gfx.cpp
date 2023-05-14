#include "gfx.hpp"
#include "glsl.hpp"
#include "math.hpp"
#include "util/deferred_init.hpp"
#include "util/util.hpp"

using Resolution = glm::vec<2, unsigned>;

namespace gfx {

struct Field_viz_config {
	Resolution particle_grid_size;
	unsigned particle_lifetime;
};

static void fieldviz_init (const Field_viz_config&);
static void fieldviz_deinit ();
static void fieldviz_ensure_least_framebuffer_size (Resolution);

// ========================= Rendering context setup & handling =========================

struct Context {
	Resolution resolution;

	SDL_Window* window;
	SDL_GLContext glcontext;

	explicit Context (const Config& cfg)
		: resolution{ cfg.screen_res_x, cfg.screen_res_y }
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			FATAL("Failed to initialize SDL: {}", SDL_GetError());

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		if (cfg.msaa_samples)
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, cfg.msaa_samples);

		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		window = SDL_CreateWindow(nullptr,
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				resolution.x, resolution.y,
				SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		if (window == nullptr)
			FATAL("Failed to create SDL window: {}", SDL_GetError());

		glcontext = SDL_GL_CreateContext(window);
		if (!glcontext)
			FATAL("Failed to create SDL context: {}", SDL_GetError());

		glewExperimental = true;
		if (glewInit() != GLEW_OK)
			FATAL("Failed to initialize GLEW");

		SDL_GL_SetSwapInterval(1);

		if (cfg.msaa_samples)
			glEnable(GL_MULTISAMPLE);

		glEnable(GL_BLEND);

		SDL_SetWindowTitle(window, "Vector fields");

		if (cfg.debug) {
			MESSAGE("Enabling verbose OpenGL debugging");
			const auto callback = [] (
					[[maybe_unused]] GLenum src, [[maybe_unused]] GLenum type,
					[[maybe_unused]] GLuint id, [[maybe_unused]] GLenum severe,
					[[maybe_unused]] GLsizei len, [[maybe_unused]] const char* msg,
					[[maybe_unused]] const void* param) -> void {
				constexpr auto format = "OpenGL: {}";
				switch (severe) {
				case GL_DEBUG_SEVERITY_HIGH:
					FATAL(format, msg);
					break;
				case GL_DEBUG_SEVERITY_MEDIUM:
					WARNING(format, msg);
					break;
				case GL_DEBUG_SEVERITY_LOW:
				case GL_DEBUG_SEVERITY_NOTIFICATION:
					MESSAGE(format, msg);
					break;
				}
			};
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(callback, nullptr);
		}

		MESSAGE("Renderer is '{}' by '{}'", gl::get_string(GL_RENDERER), gl::get_string(GL_VENDOR));
		gl::poll_errors_and_die("context init");

		{ // Initialize field vizualization
			constexpr int default_spacing = 2;
			Field_viz_config fvcfg = {
				.particle_grid_size { cfg.particles_x, cfg.particles_y },
				.particle_lifetime = cfg.particle_lifetime
			};
			for (int i: { 0, 1 }) {
				if (fvcfg.particle_grid_size[i] == 0)
					fvcfg.particle_grid_size[i] = resolution[i] / default_spacing;
			}
			fieldviz_init(fvcfg);
			fieldviz_ensure_least_framebuffer_size(resolution);
		}
	}

	~Context () {
		fieldviz_deinit();

		gl::poll_errors_and_die("context deinit");

		SDL_GL_DeleteContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void update_resolution (Resolution res) {
		this->resolution = res;
		fieldviz_ensure_least_framebuffer_size(res);
	}
};

// ================================ Field visualization ================================

struct Field_viz {
	Resolution grid_size;
	unsigned get_total_particles () const { return grid_size.x * grid_size.y; }

	unsigned current_tick = 0;

	// Particles are stored in a linear buffer (left->right, top->bottom). Each particle
	// has a "head" and a "tail", which are 2d points. Those are used both to draw the
	// particle and to calculate its new position in a compute pass
	// Particle coordinates are such that neighbors in the grid are 1 apart

	gl::Buffer particles_buffer;
	gl::Vertex_array lines_vao;

	gl::Program draw_particles_program;

	// Compute shader
	gl::Program update_particles_program;
	// Matches the size specified in the shader
	constexpr static Resolution workgroup_size = { 32, 32 };
	Resolution get_dispatch_size () const { return grid_size / workgroup_size; }

	// For a cooler effect, we paint on top of what was drawn on the previous frame.
	// For the contents of the framebuffer to be well-defined at frame start, we have
	// to own the framebuffer, otherwise they are undefined
	// (and do become garbage in practice, in the absence of a compositor)
	Resolution accum_fbo_size = { 0, 0 };
	gl::Framebuffer accum_fbo;
	gl::Renderbuffer accum_rbo;

	unsigned particle_lifetime = 200;

	// Things that act upon the field are represented in a uniform buffer,
	// the format of which is one `GPU_actors` struct
	// There are vortices (clockwise with force<0) and pushers (pullers when force<0)
	struct GPU_actors {
		static constexpr int max_vortices = 16;
		static constexpr int max_pushers = 16;
		struct alignas(16) Vortex { vec2 position; float force; };
		struct alignas(16) Pusher { vec2 position; float force; };
		Vortex vortices[max_vortices];
		Pusher pushers[max_pushers];
	};
	gl::Buffer actors_buffer;
	GPU_actors* actors_buffer_mapped; // mapped write-only
	unsigned num_vortices = 0;
	unsigned num_pushers = 0;

	explicit Field_viz (const Field_viz_config& cfg)
		: grid_size { cfg.particle_grid_size }, particle_lifetime { cfg.particle_lifetime }
	{
		// Round the grid size down to workgroup size. TODO handle this more gracefully?
		grid_size /= workgroup_size;
		grid_size *= workgroup_size;

		if (unsigned num_particles = get_total_particles())
			MESSAGE("Simulating {}x{} = {} particles", grid_size.x, grid_size.y, num_particles);
		else
			FATAL("The number of particles got rounded down to zero. Try larger grid");

		{ // VBO
			particles_buffer = gl::create_buffer();
			glNamedBufferStorage(particles_buffer.get(),
					2*sizeof(vec2)*get_total_particles(), nullptr, 0);
		}

		{ // VAO & vertex format
			lines_vao = gl::gen_vertex_array();
			glBindVertexArray(lines_vao.get());

			glEnableVertexAttribArray(0);
			glVertexAttribBinding(0, 0);
			glVertexAttribFormat(0, 2, GL_FLOAT, false, 0);

			glBindVertexBuffer(0, particles_buffer.get(), 0, sizeof(vec2));
		}

		{ // SSBOs and UBOs
			actors_buffer = gl::create_buffer();
			glNamedBufferStorage(actors_buffer.get(), sizeof(GPU_actors), nullptr,
					GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
			actors_buffer_mapped = gl::map_buffer_range_as<GPU_actors>
				(actors_buffer, 0, sizeof(GPU_actors),
				 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

			gl::bind_ubo(gl::UBO_binding_point::fieldviz_actors, actors_buffer);
			gl::bind_ssbo(gl::SSBO_binding_point::fieldviz_particles, particles_buffer);
		}

		draw_particles_program = gl::Program::from_frag_vert("lines.frag", "lines.vert");
		update_particles_program = gl::Program::from_compute("particle.comp");

		gl::poll_errors_and_die("field viz init");
	}

	~Field_viz () {
		gl::unmap_buffer(actors_buffer);
	}

	void advance_simulation () {
		{ // Update mapped buffer data
			auto& m = *actors_buffer_mapped;
			float w = grid_size.x;
			float h = grid_size.y;
			float sec = current_tick / 60.0f;

			num_vortices = 0;
			num_pushers = 0;
			const auto add_vortex = [&] (float x, float y, float f) {
				m.vortices[num_vortices++] = { .position = { x, y }, .force = f };
			};
			const auto add_pusher = [&] (float x, float y, float f) {
				m.pushers[num_pushers++] = { .position = { x, y }, .force = f };
			};

			add_vortex(w*0.5, h*0.5, 200);
			add_vortex(w*0.2, h*0.1, 70*sin(sec * 0.5));
			add_vortex(w*0.3, h*0.3, 70*cos(sec * 0.5));

			add_pusher(w*0.3, h*0.9, 200*sin(sec));
			add_pusher(w*0.7, h*0.5, 75 + 75*sin(sec * 1.5f));
		}

		glUseProgram(update_particles_program.get());

		{ // Upload uniform data
			constexpr GLint unif_loc_tick = 0;
			constexpr GLint unif_loc_particle_lifetime = 1;
			constexpr GLint unif_loc_num_vortices = 10;
			constexpr GLint unif_loc_num_pushers = 11;
			glUniform1ui(unif_loc_tick, current_tick);
			glUniform1ui(unif_loc_particle_lifetime, particle_lifetime);
			glUniform1ui(unif_loc_num_vortices, num_vortices);
			glUniform1ui(unif_loc_num_pushers, num_pushers);
		}

		gl::flush_mapped_buffer_range(actors_buffer,
				offsetof(GPU_actors, vortices), sizeof(GPU_actors::Vortex) * num_vortices);
		gl::flush_mapped_buffer_range(actors_buffer,
				offsetof(GPU_actors, pushers), sizeof(GPU_actors::Pusher) * num_pushers);

		Resolution dispatch_size = get_dispatch_size();
		glDispatchCompute(dispatch_size.x, dispatch_size.y, 1);

		current_tick++;
	}

	void ensure_least_framebuffer_size (Resolution required_size) {
		if (accum_fbo_size.x >= required_size.x && accum_fbo_size.y >= required_size.y)
			return;

		constexpr Resolution max_size = { 3840, 2160 };
		if (required_size.x > max_size.x || required_size.y > max_size.y) {
			FATAL("Tried to resize framebuffer to at least {}, which is too large (max {})",
			      required_size, max_size);
		}

		// Heuristic for new framebuffer size: at first request an exact amount, after that
		// use whichever power of 2 is large enough (but still not too large)
		for (int i = 0; i < 2; i++) {
			if (accum_fbo_size[i] == 0) {
				accum_fbo_size[i] = required_size[i];
			} else {
				unsigned next_po2 = 1u << std::bit_width(required_size[i]);
				accum_fbo_size[i] = glm::clamp(accum_fbo_size[i], next_po2, max_size[i]);
			}
		}

		accum_fbo = gl::gen_framebuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, accum_fbo.get());

		accum_rbo = gl::gen_renderbuffer();
		glBindRenderbuffer(GL_RENDERBUFFER, accum_rbo.get());
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, accum_fbo_size.x, accum_fbo_size.y);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_RENDERBUFFER, accum_rbo.get());

		if (GLenum s = glCheckFramebufferStatus(GL_FRAMEBUFFER); s != GL_FRAMEBUFFER_COMPLETE)
			FATAL("Framebuffer {0} is incomplete: status {1} ({1:x})", accum_fbo.get(), s);
	}

	void draw (Resolution res, bool should_clear) const {
		glBindFramebuffer(GL_FRAMEBUFFER, accum_fbo.get());
		glViewport(0, 0, res.x, res.y);

		if (should_clear) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		glUseProgram(draw_particles_program.get());

		{ // Upload uniforms
			constexpr GLint unif_loc_workgroup_size = 0;
			constexpr GLint unif_loc_workgroup_num = 2;
			constexpr GLint unif_loc_scale = 4;

			Resolution dispatch_size = get_dispatch_size();
			glUniform2ui(unif_loc_workgroup_size, workgroup_size.x, workgroup_size.y);
			glUniform2ui(unif_loc_workgroup_num, dispatch_size.x, dispatch_size.y);

			// If viewport is too wide, cut off top & bottom; if too tall, cut off left & right
			float aspect = (float) grid_size.x * res.y / (grid_size.y * res.x);
			glUniform2f(unif_loc_scale, std::max(1.0f, aspect), std::max(1.0f, 1.0f/aspect));
		}

		glBindVertexArray(lines_vao.get());
		glDrawArrays(GL_LINES, 0, 2 * get_total_particles());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, accum_fbo.get());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, res.x, res.y, 0, 0, res.x, res.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
};

// ============================= Shallow free function API =============================

static Deferred_init<Context> global_render_context;
static Deferred_init_unchecked<Field_viz> global_fieldviz;

void init (const Config& cfg)
{
	global_render_context.init(cfg);
}

void deinit ()
{
	global_render_context.deinit();
}

void handle_sdl_event (const SDL_Event& event)
{
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
		global_render_context->update_resolution({ event.window.data1, event.window.data2 });
}

void present_frame ()
{
	gl::poll_errors_and_warn("latest frame");
	SDL_GL_SwapWindow(global_render_context->window);
}

static void fieldviz_init (const Field_viz_config& cfg)
{
	global_fieldviz.init(cfg);
}

static void fieldviz_deinit ()
{
	global_fieldviz.deinit();
}

void fieldviz_draw (bool should_clear)
{
	global_fieldviz->draw(global_render_context->resolution, should_clear);
}

static void fieldviz_ensure_least_framebuffer_size (Resolution required_size)
{
	global_fieldviz->ensure_least_framebuffer_size(required_size);
}

void fieldviz_update ()
{
	global_fieldviz->advance_simulation();
}

} // namespace gfx
