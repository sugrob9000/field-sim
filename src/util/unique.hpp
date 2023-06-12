#pragma once

#include <cassert>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

// Unique_handle:
//
// Like std::unique_ptr, but for arbitrary integer handles, not just pointers.
// Does not provide pointer passthrough semantics (so even if Id is a pointer, it should
// act like an opaque handle).
// Still requires one possible "null" value, but it can be any value (0, -1, etc.)
//
// A stateless deleter is required. Function and function pointer types are not stateless.
// Their type only represents the signature, which is not enough information for a call.
// Example of a statless deleter:
//
//   struct My_C_API_deleter {
//     operator() (void* p) const { C_API_delete(p); }
//   };
//
// Though nothing in theory prevents the support for stateful deleters, in the current
// implementation, nothing other than the underlying handle is stored.


// Unique_array:
//
// Wrapper over std::unique_ptr<T[]> + size. Also supports only stateless deleters so far.
// For when the array should be dynamically allocated, but not dynamically resized,
// avoiding the associated overhead (additional pointer and exponential growth).
//
// Supports deep constness and iteration.
//
// The interface is similar to that of `unique_ptr`: the type only knows how to delete the
// contents, not how they were created. This means:
//  - it is up to the user to pass pointers and sizes suitable for that to the constructor.
//  - no copying, no construction from range/iterator pair, etc.
// So, it is advised to use `make_array`, which ties creation and deletion back together,
// like std::make_unique does.
// TODO: `make_array` from range or iterator pair,
//    OR, better, change `Unique_array` to use allocators instead of deleters


namespace detail {
template <typename T> concept Handle = std::integral<T> || std::is_pointer_v<T>;
template <typename T, typename Id> concept Stateless_deleter
	= Handle<Id> && std::is_empty_v<T>
	&& !std::is_pointer_v<T>   // pointers to functions aren't statless
	&& !std::is_function_v<T>; // function types cannot be meaningfully stored, + see above
}

template <auto F> struct Simple_deleter {
	void operator() (detail::Handle auto id) const noexcept { F(id); }
};

template <detail::Handle Id, detail::Stateless_deleter<Id> Deleter, Id null_handle = Id{}>
class Unique_handle {
	Id id = null_handle;
	[[nodiscard]] Id disown () {
		Id result = id;
		id = null_handle;
		return result;
	}
public:
	using value_type = Id;
	using deleter_type = Deleter;

	Unique_handle () = default;
	explicit Unique_handle (Id id_): id{ id_ } { }
	Unique_handle (const Unique_handle&) = delete;
	Unique_handle (Unique_handle&& other) noexcept: id{ other.disown() } { }
	Unique_handle& operator= (Id id_) { this->reset(id_); return *this; }
	Unique_handle& operator= (const Unique_handle&) = delete;
	Unique_handle& operator= (Unique_handle&& other) noexcept {
		if (this != &other) {
			(void) release();
			id = other.disown();
		}
		return *this;
	}
	~Unique_handle () { (void) release(); }

	Deleter get_deleter () const { return {}; }

	[[nodiscard]] Id release () {
		if (id) get_deleter()(id);
		return disown();
	}

	[[nodiscard]] Id operator* () const { return this->get(); }
	// operator-> makes no sense, there are no indirection passthrough semantics

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
	auto operator<=> (const Unique_handle& other) const = default;
};


template <typename T, detail::Stateless_deleter<T> Deleter = std::default_delete<T>>
class Unique_array {
	std::unique_ptr<T[]> storage{};
	size_t len = 0;

public:
	using value_type = T;
	using size_type = size_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = pointer;
	using const_iterator = const_pointer;

	Unique_array () = default;
	void reset () { storage.reset(); len = 0; }

	Unique_array (T* ptr, size_t len_): storage(ptr), len{len_} {}
	void reset (T* ptr, size_t len_) { storage.reset(ptr); len = len_; }

	Unique_array (Unique_array&& other)
		: storage{ std::move(other.storage) }, len{ std::exchange(other.len, 0) } {}
	void reset (Unique_array&& other) {
		storage = std::move(other.storage);
		len = std::exchange(other.len, 0);
	}
	Unique_array& operator= (Unique_array&& other) {
		reset(other);
		return *this;
	}

	Unique_array (const Unique_array&) = delete;
	Unique_array& operator= (const Unique_array&) = delete;
	void reset (const Unique_array&) = delete;

	[[nodiscard]] pointer data () { return storage.get(); }
	[[nodiscard]] const_pointer data () const { return storage.get(); }

	[[nodiscard]] size_t size () const { return len; }
	[[nodiscard]] bool empty () const { return len == 0; }

	[[nodiscard]] iterator begin () { return storage.get(); }
	[[nodiscard]] iterator end () { return begin() + len; }
	[[nodiscard]] const_iterator begin () const { return storage.get(); }
	[[nodiscard]] const_iterator end () const { return storage.get() + len; }

	[[nodiscard]] reference operator[] (size_t i) { return storage[i]; }
	[[nodiscard]] const_reference operator[] (size_t i) const { return storage[i]; }
};

template <typename T> auto make_array (size_t len)
{
	return Unique_array<T>(new T[len]{}, len);
}

template <typename T> auto make_array_for_overwrite (size_t len)
{
	return Unique_array<T>(new T[len], len);
}
