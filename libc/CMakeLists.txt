# libk: the freestanding C library linked into the kernel

file(GLOB_RECURSE LIBC_SOURCES CONFIGURE_DEPENDS
    stdio/*.cpp
    stdlib/*.cpp
    string/*.cpp
    ctype/*.cpp
)

add_library(k STATIC ${LIBC_SOURCES})

target_compile_options(k PRIVATE
    -O0 -g
    -Wall -Wextra -Wno-unused-variable -Wno-unused-function
    -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-pic
    -fno-unwind-tables -fno-asynchronous-unwind-tables
    -mcmodel=large -mno-red-zone
)

set_target_properties(k PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

target_compile_definitions(k PRIVATE
    __ATLOS__
    __x86_64__
    __is_libk
    __is_libc
)

# C headers for everyone, the C++ wrappers (<cstdint>, <new>, ...) only for users
target_include_directories(k
    PUBLIC include
    INTERFACE include/cpp
)
