#pragma once

#ifndef FMT_ENFORCE_COMPILE_STRING
#  define FMT_ENFORCE_COMPILE_STRING 1
#endif

#include <cassert>
#include <cstddef>
#include <fmt/format.h>
#include <type_traits>
#include <utility>
using std::size_t;

namespace detail {
template<typename First, typename... Rest>
void debug_expr(const char* prefix, [[maybe_unused]] const char* delim, First&& first, Rest&&... rest) {
  fmt::print(stderr, FMT_STRING("Debug: {}{}"), prefix, first);
  ((fmt::print(stderr, FMT_STRING("{}{}"), delim, rest)), ...);
  std::fputc('\n', stderr);
  std::fflush(stderr);
}
}  // namespace detail

#define DEBUG_EXPR(...) ::detail::debug_expr(#__VA_ARGS__ " = ", ", ", __VA_ARGS__)
#define DEBUG_EXPR_MULTILINE(...) ::detail::debug_expr(#__VA_ARGS__ ":\n", "\n", __VA_ARGS__)

#if defined(__GNUC__) || defined(__clang__)
#  define DO_PRAGMA(X) _Pragma(#X)
#  define PRAGMA_POISON(WORD) DO_PRAGMA(GCC poison WORD)
#else
#  define DO_PRAGMA(X)
#  define PRAGMA_POISON(WORD)
#endif

namespace detail {
inline void vmessage(const char* prefix, fmt::string_view format, fmt::format_args args) {
  std::fputs(prefix, stderr);
  fmt::vprint(stderr, format, args);
  std::fputc('\n', stderr);
  std::fflush(stderr);
}

template<typename Fmt, typename... Args>
void message(const char* prefix, const Fmt& format, Args&&... args) {
  vmessage(prefix, format, fmt::make_format_args(std::forward<Args>(args)...));
}
}  // namespace detail

#define FATAL(F, ...) \
  do { \
    ::detail::message("Fatal: ", FMT_STRING(F) __VA_OPT__(, ) __VA_ARGS__); \
    ::std::exit(1); \
  } while (false)
#define WARNING(F, ...) (::detail::message("Warning: ", FMT_STRING(F) __VA_OPT__(, ) __VA_ARGS__))
#define INFO(F, ...) (::detail::message("Info: ", FMT_STRING(F) __VA_OPT__(, ) __VA_ARGS__))

// A barebones pre-C++23 implementation of start_lifetime_as (missing const, _array, etc)
// Cannot be made constexpr without compiler support, otherwise works
template<typename T>
T* start_lifetime_as(void* p)
  requires std::is_trivial_v<T>
{
  // Start the lifetime of an array of bytes there
  std::byte* const bytes = new (p) std::byte[sizeof(T)];
  T* const ptr = reinterpret_cast<T*>(bytes);
  (void) *ptr;  // Tell the abstract machine that we require a T there
  return ptr;
}
