.section .init, "ax", @progbits
   /* gcc will nicely put the contents of crtend.o's .init section here. */
   popq %rbp
   ret

.section .fini, "ax", @progbits
   /* gcc will nicely put the contents of crtend.o's .fini section here. */
   popq %rbp
   ret