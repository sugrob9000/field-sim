#include "gfx.hpp"
#include <charconv>
#include <chrono>
#include <string_view>
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

gfx::Config get_config (int argc, const char* const* argv)
{
	using std::string_view;
	gfx::Config cfg;

	using Error_message = string_view;
	auto process = [&cfg] (string_view arg) -> std::optional<Error_message> {
		if (!arg.starts_with("--")) return "options start with --";
		arg = arg.substr(2);
		if (arg == "debug") {
			cfg.debug = true;
		} else if (arg == "no-debug") {
			cfg.debug = false;
		} else if (arg.starts_with("grid=")) {
			arg = arg.substr(sizeof("grid=")-1);
			size_t x = arg.find('x');
			if (x == arg.npos) return "grid resolution has format e.g. 200x200";
			struct Dimension {
				string_view str;
				unsigned* ptr;
			} dims[2] = {
				{ arg.substr(0, x), &cfg.particles_x },
				{ arg.substr(x+1),  &cfg.particles_y }
			};
			for (const auto& d: dims) {
				auto [ptr, ec] = std::from_chars(d.str.begin(), d.str.end(), *d.ptr);
				if (ec != std::errc{}) return "one of the coordinates is not a number";
			}
		} else {
			return "unknown option";
		}
		return {};
	};

	for (int i = 1; i < argc; i++) {
		string_view arg = argv[i];
		if (auto error_msg = process(arg))
			WARNING("Bad argument '{}': {}", arg, *error_msg);
	}

	return cfg;
}
} // anon namespace

int main (int argc, char** argv)
{
	gfx::init(1560, 960, get_config(argc, argv));

	for (Input_state input; !input.poll_events().should_quit; ) {
		wait_fps(60);
		if (!input.should_freeze_field)
			gfx::fieldviz_update();
		gfx::fieldviz_draw(input.should_clear_frame);
		gfx::present_frame();
	}

	gfx::deinit();
}
