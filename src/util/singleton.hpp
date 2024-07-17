#pragma once

// A RAII class that represents the initialization of a system which requires explicit
// initialization, but which are global, i.e. one instance can or should exist at a time.
// In debug builds, this is asserted at runtime.
//
// Tag should be unique per system (per lock type) so that each system gets its distinct
// flag that is checked in debug mode. Besides distinguishing instantiations of
// Singleton_lock, tags are not used for anything.
//
// Singleton_lock is intended to be inherited from by classes which initialize the system
// in their constructor and deinitialize it in their destructor, as is natural in C++.
// When inheriting, it is convenient to use the derived class itself as the tag.
//
// Example, a very barebones SDL initializer without any error checking:
//
//    struct SDL_init_lock: Singleton_lock<SDL_init_lock> {
//      SDL_init_lock () { SDL_Init(SDL_INIT_ALL); }
//      ~SDL_init_lock () { ~SDL_Quit(); }
//    };

#include <cassert>

template<typename Tag>
struct Singleton_lock {
#ifndef NDEBUG
  static inline bool initialized = false;

  Singleton_lock() {
    assert(!initialized);
    initialized = true;
  }

  ~Singleton_lock() {
    assert(initialized);
    initialized = false;
  }
#else
  Singleton_lock() = default;
  ~Singleton_lock() = default;
#endif
  Singleton_lock(auto&&) = delete;
  auto operator=(auto&&) = delete;
};
