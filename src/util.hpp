#ifndef UTIL_HPP
#define UTIL_HPP

#include <cassert>
#include <cstddef>
#include <tuple>
#include <utility>
using std::size_t;

#if 1
#define FMT_ENFORCE_COMPILE_STRING 1
#endif
#include <fmt/core.h>
#include <fmt/format.h>

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
 * Intended for use with string literals, and deliberately ignores the null terminator
 * that they always have.  `Avoid_zeros` can be used to set unused high bytes (if any)
 * to 0xFF, to avoid accidentally making a valid C-string.
 */
template <typename Uint, bool Avoid_zeros = false, size_t N>
constexpr Uint pack_chars (const char (&literal)[N])
{
	static_assert(std::is_unsigned_v<Uint>);
	constexpr size_t total = sizeof(Uint);
	constexpr size_t used = N-1;
	static_assert(N > 0 && used <= total, "Need literal that fits into the integer type");
	Uint result = 0;
	for (size_t i = 0; i < used; i++)
		result = (result << 8) | literal[i];
	if constexpr (Avoid_zeros)
		result |= ((Uint{1} << 8*(total-used))-1) << (8*used);
	return result;
}

/*
 * Wrappers for in-place deferred initialization: allocate the space up front,
 * but construct and destruct the object explicitly and at any chosen point,
 * avoiding static initialization order fiasco.
 * Mostly intended for global variables.
 */

template <typename T> class Deferred_init_unchecked {
	alignas(T) char storage[sizeof(T)];
public:
	Deferred_init_unchecked () = default;
	Deferred_init_unchecked (const Deferred_init_unchecked&) = delete;
	Deferred_init_unchecked (Deferred_init_unchecked&&) = delete;
	auto& operator= (const Deferred_init_unchecked&) = delete;
	auto& operator= (Deferred_init_unchecked&&) = delete;
	template <typename... Args> void init (Args&&... args) {
		new (storage) T(std::forward<Args>(args)...);
	}
	void deinit () { std::destroy_at(&this->operator*()); }
	T& operator* () { return *std::launder(reinterpret_cast<T*>(storage)); }
	T* operator-> () { return &this->operator*(); }
};

template <typename T> class Deferred_init: Deferred_init_unchecked<T> {
	bool initialized = false;
public:
	using Base = Deferred_init_unchecked<T>;
	template <typename... Args> void init (Args&&... args) {
		assert(!initialized);
		initialized = true;
		Base::init(std::forward<Args>(args)...);
	}
	void deinit () {
		assert(initialized);
		Base::deinit();
		initialized = false;
	}
	T& operator* () {
		assert(initialized);
		return Base::operator*();
	}
	T* operator-> () { return &this->operator*(); }
};


/*
 * Unique_handle: like std::unique_ptr, but for arbitrary integer handles,
 * necessarily with one possible "null" value and a stateless deleter.
 * TODO: Does not implement the full unique_ptr-like interface yet
 */
template <std::integral Id, typename Deleter, Id null_handle = 0>
class Unique_handle {
	static_assert(!std::is_pointer_v<Deleter> && !std::is_function_v<Deleter>,
			"The types for functions and function pointers are not monostate: "
			"they only encode the signature, which is not enough information");
	static_assert(std::is_empty_v<Deleter>, "Unique_handle only supports stateless deleters");
	Id id = null_handle;
	[[nodiscard]] Id pilfer () {
		Id result = id;
		id = null_handle;
		return result;
	}
	void release_discard () { (void) release(); }
public:
	Unique_handle () { }
	Unique_handle (Id id_): id{ id_ } { }
	Unique_handle (const Unique_handle&) = delete;
	Unique_handle (Unique_handle&& other): id{ other.pilfer() } { }
	Unique_handle& operator= (const Unique_handle&) = delete;
	Unique_handle& operator= (Unique_handle&& other) {
		if (this != &other) {
			release_discard();
			id = other.pilfer();
		}
		return *this;
	}
	~Unique_handle () { std::ignore = release(); }

	[[nodiscard]] Id release () {
		if (id) Deleter{}(id);
		return pilfer();
	}

	[[nodiscard]] Id get () const {
		assert(id != null_handle);
		return id;
	}

	void reset (Id new_id = null_handle) {
		release_discard();
		id = new_id;
	}

	void swap (Unique_handle& other) { std::swap(id, other.id); }

	operator bool () const { return id != null_handle; }
};

/*
 * A barebones pre-C++23 implementation of start_lifetime_as (missing const, _array, etc)
 * Cannot be made constexpr without compiler support, otherwise works
 */
template <typename T> concept Trivial = std::is_trivial_v<T>;
template <Trivial T> T* start_lifetime_as (void* p)
{
	/* Start the lifetime of an array of bytes there */
	std::byte* const bytes = new (p) std::byte[sizeof(T)];
	T* const ptr = reinterpret_cast<T*>(bytes);
	(void) *ptr; /* Tell the abstract machine that we require a T there */
	return ptr;
}

#endif /* UTIL_HPP */
