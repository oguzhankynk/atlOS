/**
 * @file uninitialized.hpp
 * @brief Static storage for an object that is constructed explicitly.
 *
 * The composition root (kernelMain) owns the kernel's objects. They need static
 * storage (there is no heap yet and the boot stack is small) but must not be
 * built by the global constructors, whose order is not under our control.
 * Uninitialized<T> reserves the storage and constructs T when construct() is
 * called, so the construction order is the order of the code in kernelMain.
 *
 * The object is never destroyed; the kernel does not return from kernelMain.
 */

#pragma once

#include <core/panic.hpp>
#include <new>

namespace Cxx {

template <typename T>
class Uninitialized {
public:
    // constexpr: zero initialized at load time, no global constructor
    constexpr Uninitialized() = default;

    Uninitialized(const Uninitialized&) = delete;
    Uninitialized& operator=(const Uninitialized&) = delete;

    template <typename... Args>
    T& construct(Args&&... args)
    {
        if (m_constructed)
            kernelPanic("Uninitialized: constructed twice");

        T* object = new (m_storage) T(static_cast<Args&&>(args)...);
        m_constructed = true;
        return *object;
    }

    bool isConstructed() const { return m_constructed; }

    T& get()
    {
        if (!m_constructed)
            kernelPanic("Uninitialized: used before construct()");
        return *reinterpret_cast<T*>(m_storage);
    }

    T* operator->() { return &get(); }
    T& operator*() { return get(); }

private:
    alignas(T) unsigned char m_storage[sizeof(T)] {};
    bool m_constructed { false };
};

} // namespace Cxx
