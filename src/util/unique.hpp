#pragma once

#include <cassert>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace detail {
/*
 * The types for functions and function pointers are not stateless, because the type
 * only has the signature, which is not enough information for a call.
 *
 * Example of a statless deleter:
 *
 * struct My_C_API_deleter {
 *   operator() (void* p) const { C_API_delete(p); }
 * };
 */
template <typename T, typename Id> concept Stateless_deleter
	= std::is_empty_v<T> && !std::is_pointer_v<T> && !std::is_function_v<T>;
}

/*
 * Unique_handle: like std::unique_ptr, but for arbitrary integer handles,
 * not just pointers, but still necessarily with one possible "null" value.
 *
 * A stateless deleter is required.
 * (Though nothing in theory prevents the implementation of stateful deleters.)
 * In the current implementation, nothing other than the underlying handle is stored.
 */
template <std::integral Id, detail::Stateless_deleter<Id> Deleter, Id null_handle = Id{}>
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
	/* operator-> makes no sense, there are no indirection passthrough semantics */

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


/*
 * Unique_array: wrapper over unique_ptr<T[]> + size.
 *
 * For when one needs a dynamic array without the ability to dynamically resize - and the
 * associated overhead, such as storing capacity or exponential allocation growth.
 * Can still change size via reset() or move-assignment.
 *
 * Over unique_ptr + size, also provides (some) iterator support and deep constness
 */
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

	Unique_array ();
	void reset () { storage.reset(); len = 0; }

	/*
	 * `ptr` must point to an array of `len_` elements suitable for deallocation with T,
	 * which is a tight invariant to have on the two parameters of a constructor.
	 *
	 * You should wrap this constructor in a bespoke `make_...` to maintain it,
	 * or use `make_array`, which does that for `new[]/delete[]`
	 */
	Unique_array (T* ptr, size_t len_): storage(ptr), len{len_} {}
	void reset (T* ptr, size_t len_) { storage.reset(ptr); len = len_; }

	Unique_array (Unique_array&& other)
		: storage{ std::move(other.storage) }, len{ std::exchange(other.len, 0) } {}

	[[nodiscard]] T* data () { return storage.get(); }
	[[nodiscard]] const T* data () const { return storage.get(); }

	[[nodiscard]] size_t size () const { return len; }
	[[nodiscard]] bool empty () const { return len == 0; }

	iterator begin () { return storage.get(); }
	iterator end () { return begin() + len; }
	const_iterator begin () const { return storage.get(); }
	const_iterator end () const { return storage.get() + len; }
};

template <typename T> auto make_array (size_t len)
{
	return Unique_array<T>(new T[len]{}, len);
}

template <typename T> auto make_array_for_overwrite (size_t len)
{
	return Unique_array<T>(new T[len], len);
}
