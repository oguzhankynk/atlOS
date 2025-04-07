.equ Registers.eax, 0
.equ Registers.ebx, 4
.equ Registers.ecx, 8
.equ Registers.edx, 12
.equ Registers.esi, 16
.equ Registers.edi, 20
.equ Registers.esp, 24
.equ Registers.ebp, 28
.equ Registers.eip, 32
.equ Registers.eflags, 36
.equ Registers.cr3, 40

.section .text
.global switchTask
switchTask:
    #void switchTask(Registers *from, Registers *to);

    pusha #push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI :  8 reg = 8×4 = +32 byte
    pushf #push EFLAGS #: +4 byte
    mov %cr3, %eax #Push CR3
    push %eax #:  +4 byte

    #   stack view:
    #   ESP →             [ CR3 value ]                 ← push %eax
    #   ESP+4 →           [ EFLAGS ]                    ← pushf
    #   ESP+8 →           [ EDI ]
    #   ESP+12 →          [ ESI ]
    #   ESP+16 →          [ EBP ]
    #   ESP+20 →          [ ESP (prev value) ]
    #   ESP+24 →          [ EBX ]
    #   ESP+28 →          [ EDX ]
    #   ESP+32 →          [ ECX ]
    #   ESP+36 →          [ EAX ]
    #   ESP+40 →          [ return address ]            ← call switchTask
    #   ESP+44 →          [ froRegisters *from ]                
    #   ESP+48 →          [ Registers *to ]              

    mov 44(%esp), %eax #The first argument, where to save
    mov %ebx, 4(%eax)
    mov %ecx, 8(%eax)
    mov %edx, 12(%eax)
    mov %esi, 16(%eax)
    mov %edi, 20(%eax)
    mov 36(%esp), %ebx #EAX
    mov 40(%esp), %ecx #IP
    mov 20(%esp), %edx #ESP
    add $4, %edx #Remove the return value ;)
    mov 16(%esp), %esi #EBP
    mov 4(%esp), %edi #EFLAGS
    mov %ebx, (%eax)
    mov %edx, 24(%eax)
    mov %esi, 28(%eax)
    mov %ecx, 32(%eax)
    mov %edi, 36(%eax)
    pop %ebx #CR3
    mov %ebx, 40(%eax)
    push %ebx #Goodbye again ;)

    mov 48(%esp), %eax #Now it is the new object
    mov 4(%eax), %ebx #EBX
    mov 8(%eax), %ecx #ECX
    mov 12(%eax), %edx #EDX
    mov 16(%eax), %esi #ESI
    mov 20(%eax), %edi #EDI
    mov 28(%eax), %ebp #EBP

    push %eax
    mov 36(%eax), %eax #EFLAGS
    push %eax
    popf
    pop %eax
    mov 24(%eax), %esp #ESP
    push %eax

    mov 40(%eax), %eax #CR3
    mov %eax, %cr3
    pop %eax
    
    push %eax
    mov 32(%eax), %eax #EIP
    xchg (%esp), %eax #We do not have any more registers to use as tmp storage
    mov (%eax), %eax #EAX
    ret #This ends all!
