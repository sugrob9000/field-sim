#include "gfx.hpp"
#include "util/util.hpp"
#include <chrono>
#include <thread>
#include <vector>

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

static void wait_fps (int fps)
{
	static auto next = std::chrono::steady_clock::now();
	next += std::chrono::duration<long, std::micro>{ 1000'000 / fps };
	std::this_thread::sleep_until(next);
}
} // anon namespace

int main ()
{
	gfx::init(1560, 960, { .debug = false, .msaa_samples = 4 });

	for (Input_state input; !input.poll_events().should_quit; ) {
		wait_fps(60);
		if (!input.should_freeze_field)
			gfx::fieldviz_update();
		gfx::fieldviz_draw(input.should_clear_frame);
		gfx::present_frame();
	}

	gfx::deinit();
}
