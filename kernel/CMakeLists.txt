# atlOS.kernel: the multiboot2 kernel image

set(ARCHDIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64)

set(KERNEL_PHYSICAL_START 0x0000000000400000)
set(KERNEL_VIRTUAL_LOAD_OFFSET 0xFFFFFFFF80000000)

# ------------------------------------------------------------
# Flags shared by every kernel object (C, C++ and assembly)
# ------------------------------------------------------------

add_library(kernel_flags INTERFACE)

target_compile_options(kernel_flags INTERFACE
    -g
    -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-function
    -Wno-missing-field-initializers -Wno-maybe-uninitialized
    -Wno-format -Wno-format-security -Wno-unused-parameter
    -Wno-sign-compare -Wno-implicit-fallthrough
    -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-pic
    -fno-unwind-tables -fno-asynchronous-unwind-tables
    -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
)

target_compile_definitions(kernel_flags INTERFACE
    __is_kernel
    __ATLOS__
    __x86_64__
    KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}
    KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}
    $<$<BOOL:${ATLOS_SELFTEST}>:ATLOS_SELFTEST>
)

# Includes are relative to kernel/, e.g. <mm/vmm.hpp>; libc headers come with k
target_include_directories(kernel_flags INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

file(GLOB_RECURSE KERNEL_SOURCES CONFIGURE_DEPENDS
    core/*.cpp
    cxx/*.cpp
    dev/*.cpp
    drivers/*.cpp
    firmware/*.cpp
    fs/*.cpp
    mm/*.cpp
    sched/*.cpp
    sync/*.cpp
    tests/*.cpp
    ${ARCHDIR}/gdt/*.cpp
    ${ARCHDIR}/idt/*.cpp
    ${ARCHDIR}/idt/isr.S
    ${ARCHDIR}/io/*.cpp
    ${ARCHDIR}/cpu/context.S
)
# Not built (yet): arch/x86_64/cpu/cpu.cpp, cpuid.cpp

# Linked in a fixed order around the other objects, see the link rule below
add_library(crts OBJECT
    ${ARCHDIR}/boot/boot.S
    ${ARCHDIR}/crt/crti.S
    ${ARCHDIR}/crt/crtn.S
)
target_link_libraries(crts PRIVATE kernel_flags)

add_executable(atlOS.kernel ${KERNEL_SOURCES})
target_link_libraries(atlOS.kernel PRIVATE kernel_flags k gcc)
add_dependencies(atlOS.kernel crts)

target_link_options(atlOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld
    -nostdlib -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit
    -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2
    -z max-page-size=0x1000
    "-Wl,--defsym,KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}"
    "-Wl,--defsym,KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}"
)
set_target_properties(atlOS.kernel PROPERTIES LINK_DEPENDS ${ARCHDIR}/linker.ld)

# ------------------------------------------------------------
# Link order: crti crtbegin boot <objects> <libs> crtend crtn
# ------------------------------------------------------------

execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o
    OUTPUT_VARIABLE CRTBEGIN_O
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtend.o
    OUTPUT_VARIABLE CRTEND_O
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CRTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/crts.dir/arch/x86_64)

set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <LINK_FLAGS> \
 ${CRTS_DIR}/crt/crti.S.obj \
 ${CRTBEGIN_O} \
 ${CRTS_DIR}/boot/boot.S.obj \
 <OBJECTS> \
 -o <TARGET> \
 <LINK_LIBRARIES> \
 ${CRTEND_O} \
 ${CRTS_DIR}/crt/crtn.S.obj")
