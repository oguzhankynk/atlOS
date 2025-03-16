.section .text
.extern int_handler /**< Declares an external function, exception_handler, */

.macro INT_DEFINATION NO HANDLER
    .global INT_\NO /**< Define the exception handler for exception number NO as a global symbol. */
    .align 4

    INT_\NO:

        .if !(\NO == 8 || (\NO >= 10 && \NO <= 14) || \NO == 17 || \NO == 21)
            pushl $0
            /**< If the exception don't push error code, push 0  */
        .endif

        pushl $\NO /**< Push the exception number NO onto the stack. */

        pusha /**< Push the values of all general-purpose registers onto the stack (eax, ebx, ecx, edx, ebp, esp, esi, edi). */

        xor %eax, %eax         /**<push ds*/
        mov %ds, %ax 
        push %eax

        mov $0x10 , %ax         /**<kernel data segment*/
        mov %ax, %ds
        mov %ax, %es
        mov %ax, %fs
        mov %ax, %gs

        push %esp  /**< Call by pointer */

        call \HANDLER /**< Call the exception handler specified by HANDLER. */

        pop %eax  /**< Pop the esp  */

        pop %eax            /**<pop old segment*/
        mov %ax, %ds 
        mov %ax, %es 
        mov %ax, %fs 
        mov %ax, %gs 

        popa /**< Pop the general-purpose registers from the stack (eax, ebx, ecx, edx, ebp, esp, esi, edi). */

        pop %eax  /**< Pop the exception number  */
        pop %eax  /**< Pop the error no  */

        sti /**< Set the interrupt flag to re-enable interrupts. */
        iret /**< Return from interrupt, restoring the state of the interrupted program. */
.endm


.altmacro
    .set i, 0
    .rept 9      
        INT_DEFINATION %i int_handler      
        .set i, i + 1
    .endr

.altmacro
    .set i, 10
    .rept 5      
        INT_DEFINATION %i int_handler      
        .set i, i + 1
.endr

.altmacro
    .set i, 16
    .rept 6     
        INT_DEFINATION %i int_handler      
        .set i, i + 1
.endr

.altmacro
    .set i, 32
    .rept 224      
        INT_DEFINATION %i int_handler      
        .set i, i + 1
.endr
 
