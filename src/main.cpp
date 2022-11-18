#include "gfx.hpp"
#include "math.hpp"
#include "util/util.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

struct Input_state {
	bool should_quit = false;
	bool should_clear_frame = false;
	bool freeze_field = false;
	Input_state& poll_events ();
};

void wait_fps (int fps);

int main ()
{
	gfx::init(1560, 960);

	Input_state input;
	while (!input.poll_events().should_quit) {
		wait_fps(60);
		if (!input.freeze_field)
			gfx::fieldviz_update();
		gfx::fieldviz_draw(input.should_clear_frame);
		gfx::present_frame();
	}

	gfx::deinit();
}

Input_state& Input_state::poll_events ()
{
	for (SDL_Event event; SDL_PollEvent(&event); ) {
		gfx::handle_sdl_event(event);
		switch (event.type) {
		case SDL_QUIT: should_quit = true; break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_q: should_quit = true;; break;
			case SDLK_c: should_clear_frame ^= 1; break;
			case SDLK_f: freeze_field ^= 1; break;
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

void wait_fps (int fps)
{
	namespace chrono = std::chrono;
	using Microseconds = chrono::duration<long, std::micro>;
	static auto last = chrono::steady_clock::now();
	auto end = last + Microseconds{ 1000'000 / fps };
	std::this_thread::sleep_until(end);
	last = chrono::steady_clock::now();
}
