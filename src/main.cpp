#include "gfx.hpp"
#include <charconv>
#include <chrono>
#include <string_view>
#include <thread>

namespace {
struct Input_state {
  bool should_quit = false;
  bool should_clear_frame = false;
  bool should_update_field = true;

  Input_state& poll_events() {
    for (SDL_Event event; SDL_PollEvent(&event);) {
      gfx::handle_sdl_event(event);
      switch (event.type) {
      case SDL_QUIT:
        should_quit = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_q:
          should_quit = true;
          break;
        case SDLK_c:
          should_clear_frame ^= 1;
          break;
        case SDLK_f:
          should_update_field ^= 1;
          break;
        case SDLK_d:
          if (event.key.keysym.mod & KMOD_SHIFT) {
            asm("int3" :::);
          }
          break;
        }
        break;
      }
    }
    return *this;
  }
};

void wait_fps(int fps) {
  static auto next = std::chrono::steady_clock::now();
  next += std::chrono::microseconds{1000'000 / fps};
  std::this_thread::sleep_until(next);
}

namespace arg {
  using std::string_view;

  struct Arg_parse_exception {
    string_view subject;
    string_view defect;
  };

  void parse_number(string_view arg, auto& x) {
    auto [ptr, ec] = std::from_chars(arg.begin(), arg.end(), x);
    if (ec != std::errc{}) {
      throw Arg_parse_exception{.subject = arg, .defect = "is not a number"};
    }
  }

  void parse_resolution(string_view arg, auto& x, auto& y) {
    size_t delim = arg.find('x');
    if (delim == arg.npos) {
      throw Arg_parse_exception{.subject = arg, .defect = "has no delimiter (e.g. 200x200)"};
    }
    parse_number(arg.substr(0, delim), x);
    parse_number(arg.substr(delim + 1), y);
  }

  gfx::Config get_config(int argc, const char* const* argv) {
    gfx::Config cfg;

    const auto process_argument = [&cfg](string_view arg) {
      if (!arg.starts_with("--")) {
        throw Arg_parse_exception{.subject = arg, .defect = "does not start with --"};
      }
      arg = arg.substr(2);
      if (arg == "debug") {
        cfg.debug = true;
      } else if (arg == "no-debug") {
        cfg.debug = false;
      } else if (arg.starts_with("res=")) {
        parse_resolution(arg.substr(sizeof("res=") - 1), cfg.screen_res_x, cfg.screen_res_y);
      } else if (arg.starts_with("grid=")) {
        parse_resolution(arg.substr(sizeof("grid=") - 1), cfg.particles_x, cfg.particles_y);
      } else if (arg.starts_with("life=")) {
        parse_number(arg.substr(sizeof("life=") - 1), cfg.particle_lifetime);
      } else if (arg.starts_with("spacing=")) {
        parse_number(arg.substr(sizeof("spacing=") - 1), cfg.particle_spacing);
      } else {
        throw Arg_parse_exception{.subject = arg, .defect = "is not a valid option"};
      }
    };

    for (int i = 1; i < argc; i++) {
      string_view arg = argv[i];
      try {
        process_argument(arg);
      } catch (Arg_parse_exception& ex) {
        WARNING("Bad argument '{}': '{}' {}", arg, ex.subject, ex.defect);
      }
    }

    return cfg;
  }
}  // namespace arg
}  // namespace

int main(int argc, char** argv) {
  gfx::Init_lock gfx(arg::get_config(argc, argv));

  for (Input_state input; !input.poll_events().should_quit;) {
    wait_fps(60);
    if (input.should_update_field) {
      gfx::fieldviz_update();
    }
    gfx::fieldviz_draw(input.should_clear_frame);
    gfx::present_frame();
  }
}
