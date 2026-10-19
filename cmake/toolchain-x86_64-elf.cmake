# Cross toolchain for atlOS (x86_64-elf, freestanding).
# The x86_64-elf-* tools must be on PATH, see docs/building.md.

set(CMAKE_SYSTEM_NAME atlOS)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Makes CMake pick up Platform/atlOS.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_C_COMPILER x86_64-elf-gcc)
set(CMAKE_CXX_COMPILER x86_64-elf-g++)
set(CMAKE_ASM_COMPILER x86_64-elf-gcc)
set(CMAKE_LINKER x86_64-elf-ld)

# There is no C runtime to link a test executable against
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
