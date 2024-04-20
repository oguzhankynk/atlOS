cmake_minimum_required(VERSION 3.22.1)

# Include the ParsOS platform-specific settings
include(${CMAKE_MODULE_PATH}/Platform/ParsOS.cmake)

# Check if SYSROOT is defined
if(NOT DEFINED SYSROOT)
    # Print error message and stop the configuration process
    message(FATAL_ERROR "SYSROOT is not defined. Please define SYSROOT to specify the target root directory.")
endif()


# Define the project name and specify that the project uses C and C++ languages
project(ParsOS_kernel C CXX ASM)

# Specify compiler flags for C and C++ sources
SET (CMAKE_CXX_FLAGS "-std=c++11 -m32 -ffreestanding -fno-rtti -fno-exceptions -nostdlib -nostdinc -fno-stack-protector")
SET (CMAKE_C_FLAGS "-m32 -ffreestanding -fno-exceptions -nostdlib -nostdinc -fno-stack-protector")
set(CMAKE_ASM_FLAGS "-m32 -nostdlib -nostdinc ")

# Define the ARCHDIR as kernel/arch/i386
set(ARCHDIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386)

file(GLOB_RECURSE KERNEL_FILES
    ${ARCHDIR}/**/*.c 
    ${ARCHDIR}/**/*.cpp
    ${ARCHDIR}/**/*.S
    kernel/banner.cpp
)

# Print the list of found objects
message(STATUS "Found kernel objects: ${KERNEL_FILES}")

# Add include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSROOT}/usr/include
)

add_library(kernel ${KERNEL_FILES})

# Create the ParsOS.kernel executable using the libkernel.a, libgcc.a, and libk.a libraries
add_executable(ParsOS.kernel kernel/kernel.cpp)

target_link_options(ParsOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld
)

# Link order and list for the executable
target_link_libraries(ParsOS.kernel PRIVATE
    kernel
    k
    gcc
)

target_link_directories(ParsOS.kernel PRIVATE
    ${SYSROOT}/usr/lib
)

# Add a custom command to copy ParsOS.kernel to SYSROOT/boot
add_custom_command(
    TARGET ParsOS.kernel POST_BUILD
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/ParsOS.kernel ${SYSROOT}/boot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Install header files from include/kernel/public to SYSROOT/usr/include/public
install( DIRECTORY include/kernel/public DESTINATION ${SYSROOT}/usr/include/kernel )

# Install header files from include/common to SYSROOT/usr/include/common
install(DIRECTORY include/common DESTINATION ${SYSROOT}/usr/include)