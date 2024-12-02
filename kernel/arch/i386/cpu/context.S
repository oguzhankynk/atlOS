.set TCB_esp,   0
.set TCB_ebp,   4
.set TCB_ebx,   8
.set TCB_esi,   12
.set TCB_edi,   16
.set TCB_eip,   20
.set TCB_cr3,   24
.set TCB_eflag, 28


# Call in C with switchCPUContext(TCB* currentThread, TCB nextThread);

.section .text
.global switchCPUContext
switchCPUContext:
  # save currentThread registers

    movl 4(%esp), %edi        # currentThread = %edi
    movl %esp, TCB_esp(%edi)  # currentThread->esp = esp
    movl %ebx, TCB_ebx(%edi)  # currentThread->ebx = ebx
    movl %esi, TCB_esi(%edi)  # currentThread->esi = esi
    movl %edi, TCB_edi(%edi)  # currentThread->edi = edi
    movl %ebp, TCB_ebp(%edi)  # currentThread->ebp = ebp

    lea 1f, %eax          
    movl %eax, TCB_eip(%edi)  # currentThread->eip = eip
   
    movl 8(%esp), %esi         # nextThread
    movl TCB_esp(%esi), %esp   # esp = nextThread->esp
    movl TCB_ebx(%esi), %ebx   # ebx = nextThread->ebx
    movl TCB_esi(%esi), %esi   # esi = nextThread->esi
    movl TCB_edi(%esi), %edi   # edi = nextThread->edi
    movl TCB_ebp(%esi), %ebp   # ebp = nextThread->ebp
    movl TCB_eip(%esi), %eax   # eax = nextThread->eip
    jmp *%eax  # Yeni iş parçacığını başlat

    #ret    
1:                  