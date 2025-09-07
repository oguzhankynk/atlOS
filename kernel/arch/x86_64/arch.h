

#if defined(__x86_64__)
#define KERNEL_LMA_BASE 0x0000000000010000
#define KERNEL_VMA_BASE 0xFFFFFFFF80000000
#error "Unsupported architecture"
#else
#error "Unsupported architecture"
#endif

#define KERNEL_LMA_BASE 0x0000000000010000
#define KERNEL_VMA_BASE 0xFFFFFFFF80000000
