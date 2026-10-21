/**
 * @file raise.cpp
 * @author Oguzhan KAYNAK
 * @brief Raising any interrupt vector in software ("int n").
 */

#include <arch/x86_64/idt/raise.hpp>

namespace IDT {

// "int" takes the vector as an immediate, so there is one function per vector,
// made by the compiler from this template
template <int Vector>
static void raiseVector()
{
    if constexpr (hasErrorCode(Vector)) {
        // "int n" pushes no error code, but this vector's stub expects one
        // right below rip. So enter the stub the way the CPU would: align the
        // stack, push ss, rsp, rflags, cs, the return address and a 0 error
        // code, jump to the stub; its iretq comes back to label 1.
        __asm__ volatile(
            "movq %%rsp, %%rax\n\t"
            "andq $-16, %%rsp\n\t"
            "pushq $0\n\t" // ss (the kernel runs with a null ss)
            "pushq %%rax\n\t" // rsp
            "pushfq\n\t" // rflags
            "pushq $0x08\n\t" // cs: kernel code
            "leaq 1f(%%rip), %%rax\n\t"
            "pushq %%rax\n\t" // rip
            "pushq $0\n\t" // error code
            "jmp INT_%c0\n"
            "1:"
            :
            : "i"(Vector)
            : "rax", "memory");
    } else {
        __asm__ volatile("int %0"
                         :
                         : "i"(Vector)
                         : "memory");
    }
}

// 0, 1, ..., 255 as a parameter pack
template <int... Vectors>
struct VectorList {
};

template <int Count, int... Vectors>
struct MakeVectorList : MakeVectorList<Count - 1, Count - 1, Vectors...> {
};

template <int... Vectors>
struct MakeVectorList<0, Vectors...> {
    using Type = VectorList<Vectors...>;
};

using RaiseFunction = void (*)();

template <int... Vectors>
struct RaiseTable {
    static constexpr RaiseFunction functions[sizeof...(Vectors)] = { raiseVector<Vectors>... };
};

template <int... Vectors>
static constexpr const RaiseFunction* raiseTable(VectorList<Vectors...>)
{
    return RaiseTable<Vectors...>::functions;
}

void raise(uint8_t vector)
{
    static constexpr const RaiseFunction* table = raiseTable(MakeVectorList<256>::Type {});
    table[vector]();
}

} // namespace IDT
