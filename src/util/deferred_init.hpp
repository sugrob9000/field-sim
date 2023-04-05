#ifndef UTIL_DEFERRED_INIT_HPP
#define UTIL_DEFERRED_INIT_HPP

#include <cassert>
#include <memory>
#include <new>
#include <utility>

/*
 * Wrappers for in-place deferred initialization: allocate the space up front,
 * but construct and destruct the object explicitly and at any chosen point.
 * Mostly intended for global variables, to avoid static initialization order fiasco,
 * but could be useful for data members also.
 *
 * The checked version is essentially std::optional, but with clearer intent
 * and no copy- or move-construction or -assignment at all
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

#endif /* UTIL_DEFERRED_INIT_HPP */
