#include "gfx.hpp"
#include <chrono>
#include <thread>

namespace {
struct Input_state {
	bool should_quit = false;
	bool should_clear_frame = false;
	bool should_freeze_field = false;

	Input_state& poll_events () {
		for (SDL_Event event; SDL_PollEvent(&event); ) {
			gfx::handle_sdl_event(event);
			switch (event.type) {
				case SDL_QUIT: should_quit = true; break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_q: should_quit = true; break;
						case SDLK_c: should_clear_frame ^= 1; break;
						case SDLK_f: should_freeze_field ^= 1; break;
						case SDLK_d:
							if (event.key.keysym.mod & KMOD_SHIFT)
								asm("int3":::);
						break;
					}
					break;
			}
		}
		return *this;
	}
};

void wait_fps (int fps)
{
	static auto next = std::chrono::steady_clock::now();
	next += std::chrono::microseconds{ 1000'000 / fps };
	std::this_thread::sleep_until(next);
}
} // anon namespace

int main (int argc, char** argv)
{
	gfx::Config gfx_config = { .debug = false, .msaa_samples = 4 };

	{ // Crudely parse arguments
		for (int i = 1; i < argc; i++) {
			std::string_view arg = argv[i];
			if (arg == "debug")
				gfx_config.debug = true;
			else if (arg == "nodebug")
				gfx_config.debug = false;
		}
	}

	gfx::init(1560, 960, gfx_config);

	for (Input_state input; !input.poll_events().should_quit; ) {
		wait_fps(60);
		if (!input.should_freeze_field)
			gfx::fieldviz_update();
		gfx::fieldviz_draw(input.should_clear_frame);
		gfx::present_frame();
	}

	gfx::deinit();
}
