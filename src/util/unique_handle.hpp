#ifndef UTIL_UNIQUE_HANDLE_HPP
#define UTIL_UNIQUE_HANDLE_HPP

#include <cassert>
#include <concepts>
#include <type_traits>

/*
 * Unique_handle: like std::unique_ptr, but for arbitrary integer handles, necessarily
 * with one possible "null" value and a stateless deleter. (Neither an additional bool nor
 * the deleter are stored.) TODO: Does not implement the full unique_ptr-like interface
 */
template <std::integral Id, typename Deleter, Id null_handle = 0>
class Unique_handle {
	static_assert(!std::is_pointer_v<Deleter> && !std::is_function_v<Deleter>,
			"The types for functions and function pointers are not monostate: "
			"they only encode the signature, which is not enough information to call them");
	static_assert(std::is_empty_v<Deleter>, "Unique_handle only supports stateless deleters");
	Id id = null_handle;
	[[nodiscard]] Id disown () {
		Id result = id;
		id = null_handle;
		return result;
	}
public:
	Unique_handle () { }
	Unique_handle (Id id_): id{ id_ } { }
	Unique_handle (const Unique_handle&) = delete;
	Unique_handle (Unique_handle&& other): id{ other.disown() } { }
	Unique_handle& operator= (const Unique_handle&) = delete;
	Unique_handle& operator= (Unique_handle&& other) {
		if (this != &other) {
			(void) release();
			id = other.disown();
		}
		return *this;
	}
	~Unique_handle () { (void) release(); }

	[[nodiscard]] Id release () {
		if (id) Deleter{}(id);
		return disown();
	}

	[[nodiscard]] Id get () const {
		assert(id != null_handle);
		return id;
	}

	void reset (Id new_id = null_handle) {
		(void) release();
		id = new_id;
	}

	void swap (Unique_handle& other) { std::swap(id, other.id); }

	operator bool () const { return id != null_handle; }
};

#endif /* UTIL_UNIQUE_HANDLE_HPP */