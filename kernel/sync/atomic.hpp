/**
 * @file atomic.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::Atomic<T>: a value whose reads and writes can not be torn or
 * split, over the compiler's __atomic builtins.
 *
 * `volatile` only makes every access go to memory; `x++` on a volatile is
 * still load, add, store, and an interrupt (or another CPU) can come in
 * between. Atomic operations are one indivisible step (`lock xadd`,
 * `lock cmpxchg`, `xchg`), with sequentially consistent ordering.
 */

#pragma once

#include <stdint.h>

namespace Sync {

template <typename T>
class Atomic {
    static_assert(sizeof(T) <= sizeof(uint64_t), "only 1, 2, 4 and 8 byte atomics (no SSE for 16)");

public:
    constexpr Atomic() = default;
    constexpr explicit Atomic(T value)
        : m_value(value)
    {
    }

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T load() const { return __atomic_load_n(&m_value, __ATOMIC_SEQ_CST); }
    void store(T value) { __atomic_store_n(&m_value, value, __ATOMIC_SEQ_CST); }

    /// Store value, return the previous one.
    T exchange(T value) { return __atomic_exchange_n(&m_value, value, __ATOMIC_SEQ_CST); }

    /**
     * @brief If the value equals expected, replace it with desired and return
     * true; otherwise copy the current value into expected and return false.
     */
    bool compareExchange(T& expected, T desired)
    {
        return __atomic_compare_exchange_n(&m_value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }

    /// Add value, return the previous value (integers only).
    T fetchAdd(T value) { return __atomic_fetch_add(&m_value, value, __ATOMIC_SEQ_CST); }

    /// Subtract value, return the previous value (integers only).
    T fetchSub(T value) { return __atomic_fetch_sub(&m_value, value, __ATOMIC_SEQ_CST); }

private:
    T m_value {};
};

} // namespace Sync
