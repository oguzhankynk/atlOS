# User programs: static ELF64 executables for ring 3, copied to the test disk
# (/disk0/BIN). They use the cross compiler like the kernel, without its libc.

set(ATLOS_USER_FLAGS
    -ffreestanding -fno-pic -fno-pie -mcmodel=large ${ATLOS_NO_SIMD_FLAGS}
    -g -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -O2 -Wall -Wextra -Werror
)

# The start code and the heap, linked into every program. The heap is the
# kernel's own code (common/heap/), built here a second time for ring 3
set(ATLOS_USER_CXX_FLAGS -std=c++17 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics)

add_library(atlos_user_lib OBJECT
    lib/crt0.S
    lib/start.cpp
    lib/malloc.cpp
    lib/string.c
    ${CMAKE_SOURCE_DIR}/common/heap/heap.cpp
    ${CMAKE_SOURCE_DIR}/common/heap/fixed_region_pages.cpp
)
target_include_directories(atlos_user_lib PRIVATE lib ${CMAKE_SOURCE_DIR}/common)
target_compile_options(atlos_user_lib PRIVATE ${ATLOS_USER_FLAGS} $<$<COMPILE_LANGUAGE:CXX>:${ATLOS_USER_CXX_FLAGS}>)
# memset's loop must not become a call to memset (see string.c)
set_source_files_properties(lib/string.c PROPERTIES COMPILE_OPTIONS -fno-tree-loop-distribute-patterns)

set(ATLOS_USER_PROGRAMS "")

function(atlos_user_program name)
    add_executable(${name} programs/${name}.c $<TARGET_OBJECTS:atlos_user_lib>)
    target_include_directories(${name} PRIVATE lib)
    target_compile_options(${name} PRIVATE ${ATLOS_USER_FLAGS})
    target_link_options(${name} PRIVATE
        -nostdlib -static -T${CMAKE_CURRENT_SOURCE_DIR}/user.ld -Wl,-z,max-page-size=0x200000 -Wl,--build-id=none
    )
    set_target_properties(${name} PROPERTIES SUFFIX ".elf" LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/user.ld)
    set(ATLOS_USER_PROGRAMS ${ATLOS_USER_PROGRAMS} $<TARGET_FILE:${name}> PARENT_SCOPE)
endfunction()

atlos_user_program(hello)
atlos_user_program(fault)
atlos_user_program(badptr)
atlos_user_program(count)
atlos_user_program(break)
atlos_user_program(ask)
atlos_user_program(clock)
atlos_user_program(files)
atlos_user_program(args)
atlos_user_program(cat)
atlos_user_program(malloc)

set(ATLOS_USER_PROGRAMS ${ATLOS_USER_PROGRAMS} PARENT_SCOPE)
set(ATLOS_USER_TARGETS hello fault badptr count break ask clock files args cat malloc PARENT_SCOPE)
