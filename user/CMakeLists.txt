# User programs: static ELF64 executables for ring 3, copied to the test disk
# (/disk0/BIN). They use the cross compiler like the kernel, without its libc.

set(ATLOS_USER_FLAGS
    -ffreestanding -fno-pic -fno-pie -mcmodel=large ${ATLOS_NO_SIMD_FLAGS}
    -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -O2 -Wall -Wextra -Werror
)

add_library(atlos_user_crt OBJECT lib/crt0.S)

set(ATLOS_USER_PROGRAMS "")

function(atlos_user_program name)
    add_executable(${name} programs/${name}.c $<TARGET_OBJECTS:atlos_user_crt>)
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

set(ATLOS_USER_PROGRAMS ${ATLOS_USER_PROGRAMS} PARENT_SCOPE)
set(ATLOS_USER_TARGETS hello fault badptr PARENT_SCOPE)
