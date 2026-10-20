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
    -fno-omit-frame-pointer # rbp chain for the panic backtrace
    -mcmodel=large -mno-red-zone ${ATLOS_NO_SIMD_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
)

target_compile_definitions(kernel_flags INTERFACE
    __is_kernel
    __ATLOS__
    __x86_64__
    KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}
    KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}
    $<$<BOOL:${ATLOS_SELFTEST}>:ATLOS_SELFTEST>
    $<$<BOOL:${ATLOS_CRASHTEST}>:ATLOS_CRASHTEST>
)

if(NOT ATLOS_DEBUG_EXIT_PORT STREQUAL "")
    target_compile_definitions(kernel_flags INTERFACE ATLOS_DEBUG_EXIT_PORT=${ATLOS_DEBUG_EXIT_PORT})
endif()

# Includes are relative to kernel/, e.g. <mm/vmm.hpp>; libc headers come with k.
# Generated headers (core/version.hpp) live in the build tree.
set(KERNEL_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_include_directories(kernel_flags INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${KERNEL_GENERATED_DIR})

# core/version.hpp from the project version and git describe, refreshed on
# every build (the file only changes when the version or git state does)
add_custom_target(atlos_version
    COMMAND ${CMAKE_COMMAND}
        -DVERSION=${PROJECT_VERSION}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DOUTPUT=${KERNEL_GENERATED_DIR}/core/version.hpp
        -P ${CMAKE_SOURCE_DIR}/cmake/generate-version.cmake
    BYPRODUCTS ${KERNEL_GENERATED_DIR}/core/version.hpp
    COMMENT "Updating core/version.hpp"
    VERBATIM
)

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

file(GLOB_RECURSE KERNEL_SOURCES CONFIGURE_DEPENDS
    core/*.cpp
    cxx/*.cpp
    drivers/*.cpp
    firmware/*.cpp
    fs/*.cpp
    mm/*.cpp
    sched/*.cpp
    shell/*.cpp
    sync/*.cpp
    tests/*.cpp
    ${ARCHDIR}/gdt/*.cpp
    ${ARCHDIR}/idt/*.cpp
    ${ARCHDIR}/idt/isr.S
    ${ARCHDIR}/io/*.cpp
    ${ARCHDIR}/cpu/*.cpp
    ${ARCHDIR}/cpu/context.S
)

# Linked in a fixed order around the other objects, see the link rule below
add_library(crts OBJECT
    ${ARCHDIR}/boot/boot.S
    ${ARCHDIR}/crt/crti.S
    ${ARCHDIR}/crt/crtn.S
)
target_link_libraries(crts PRIVATE kernel_flags)

add_executable(atlOS.kernel ${KERNEL_SOURCES})
target_link_libraries(atlOS.kernel PRIVATE kernel_flags k gcc)
add_dependencies(atlOS.kernel crts atlos_version)

target_link_options(atlOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld
    -nostdlib -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit
    -mcmodel=large -mno-red-zone ${ATLOS_NO_SIMD_FLAGS}
    -z max-page-size=0x1000
    "-Wl,--defsym,KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}"
    "-Wl,--defsym,KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}"
)
set_target_properties(atlOS.kernel PROPERTIES LINK_DEPENDS ${ARCHDIR}/linker.ld)

# Fail the build if any SIMD instruction made it into the image (see ATLOS_NO_SIMD_FLAGS)
add_custom_command(TARGET atlOS.kernel POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DOBJDUMP=${CMAKE_OBJDUMP}
        -DIMAGE=$<TARGET_FILE:atlOS.kernel>
        -P ${CMAKE_SOURCE_DIR}/cmake/check-no-simd.cmake
    VERBATIM
)

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
