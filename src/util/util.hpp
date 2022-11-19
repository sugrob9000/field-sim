#ifndef UTIL_HPP
#define UTIL_HPP

#ifndef FMT_ENFORCE_COMPILE_STRING
#define FMT_ENFORCE_COMPILE_STRING 1
#endif

#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
#include <type_traits>
#include <utility>
using std::size_t;

#ifndef NDEBUG

template <typename First, typename... Rest> void debug_expr_
(const char* prefix, [[maybe_unused]] const char* delim, First&& first, Rest&&... rest)
{
	fmt::print(stderr, FMT_STRING("Debug: {}{}"), prefix, first);
	((fmt::print(stderr, FMT_STRING("{}{}"), delim, rest)), ...);
	std::fputc('\n', stderr);
	std::fflush(stderr);
}
#define DEBUG_EXPR(...) debug_expr_(#__VA_ARGS__" = ", ", ", __VA_ARGS__)
#define DEBUG_EXPR_MULTILINE(...) debug_expr_(#__VA_ARGS__":\n", "\n", __VA_ARGS__)

#else

#define DEBUG_EXPR(...)
#define DEBUG_EXPR_MULTILINE(...)

#endif /* NDEBUG */

#if defined(__GNUC__) || defined(__clang__)
#define DO_PRAGMA(X) _Pragma(#X)
#define PRAGMA_POISON(WORD) DO_PRAGMA(GCC poison WORD)
#else
#define DO_PRAGMA(X)
#define PRAGMA_POISON(WORD)
#endif

inline void vcomplain_ (const char* prefix, fmt::string_view f, fmt::format_args args)
{
	std::fputs(prefix, stderr);
	fmt::vprint(stderr, f, args);
	std::fputc('\n', stderr);
	std::fflush(stderr);
}

template <typename Fmt, typename... Args>
void fatal_ [[noreturn]] (const Fmt& f, Args&&... args)
{
	vcomplain_("Fatal: ", f, fmt::make_args_checked<Args...>(f, args...));
	std::exit(1);
}

template <typename Fmt, typename... Args>
void warning_ (const Fmt& f, Args&&... args)
{
	vcomplain_("Warning: ", f, fmt::make_args_checked<Args...>(f, args...));
}

#define FATAL(F, ...) fatal_(FMT_STRING(F) __VA_OPT__(,) __VA_ARGS__)
#define WARNING(F, ...) warning_(FMT_STRING(F) __VA_OPT__(,) __VA_ARGS__)

/*
 * A barebones pre-C++23 implementation of start_lifetime_as (missing const, _array, etc)
 * Cannot be made constexpr without compiler support, otherwise works
 */
template <typename T> T* start_lifetime_as (void* p) requires std::is_trivial_v<T>
{
	/* Start the lifetime of an array of bytes there */
	std::byte* const bytes = new (p) std::byte[sizeof(T)];
	T* const ptr = reinterpret_cast<T*>(bytes);
	(void) *ptr; /* Tell the abstract machine that we require a T there */
	return ptr;
}

#endif /* UTIL_HPP */
