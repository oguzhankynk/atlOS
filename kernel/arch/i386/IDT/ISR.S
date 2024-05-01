.section .text
    .extern isr_exception_handler /**< Declares an external function, isr_exception_handler, */

.macro EXCEPTION_DEFINATION NO HANDLER
    .global exception_\NO /**< Define the exception handler for exception number NO as a global symbol. */
    .align 4

    exception_\NO:


    .if !(\NO == 8 || (\NO >= 10 && \NO <= 14) || \NO == 17 || \NO == 21)
        pushl $0
        /**< If the exception don't push error code, push 0  */
    .endif

    pushl $\NO /**< Push the exception number NO onto the stack. */

    pusha /**< Push the values of all general-purpose registers onto the stack (eax, ebx, ecx, edx, ebp, esp, esi, edi). */

    call \HANDLER /**< Call the exception handler specified by HANDLER. */

    popa /**< Pop the general-purpose registers from the stack (eax, ebx, ecx, edx, ebp, esp, esi, edi). */

    add %esp, 0x8 /**< Adjust the stack pointer to clean up the exception number and error no. */

    sti /**< Set the interrupt flag to re-enable interrupts. */
    iret /**< Return from interrupt, restoring the state of the interrupted program. */
.endm

.altmacro
    .set i, 0

    /**< Loop 256 times, for each exception number from 0 to 255. */
    .rept 32      
        EXCEPTION_DEFINATION %i isr_exception_handler      
        .set i, i + 1
    .endr
 
