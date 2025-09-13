cmake_minimum_required(VERSION 3.22.1)

# Include the atlOS platform-specific settings
include(${CMAKE_MODULE_PATH}/Platform/atlOS.cmake)

# Check if SYSROOT is defined
if(NOT DEFINED SYSROOT)
    # Print error message and stop the configuration process
    message(FATAL_ERROR "SYSROOT is not defined. Please define SYSROOT to specify the target root directory.")
endif()


# Define the project name and specify that the project uses C and C++ languages
project(atlOS_kernel C CXX ASM)

# Specify compiler flags for C and C++ sources

SET(ATLOS_KERNEL_COMPILER_FLAGS 
"--sysroot=${SYSROOT} -isystem=/usr/include -g \
 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-function \
 -Wno-missing-field-initializers -Wno-maybe-uninitialized \
 -Wno-format -Wno-format-security -Wno-unused-parameter \
 -Wno-sign-compare -Wno-implicit-fallthrough \
 -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-pic \
 -fno-unwind-tables -fno-asynchronous-unwind-tables \
 -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
 -D__is_kernel -D__ATLOS__ -D__x86_64__ \
")

SET (CMAKE_CXX_FLAGS "${ATLOS_KERNEL_COMPILER_FLAGS} -std=c++17 ")

SET (CMAKE_C_FLAGS "${ATLOS_KERNEL_COMPILER_FLAGS}")

SET (CMAKE_ASM_FLAGS "${ATLOS_KERNEL_COMPILER_FLAGS} ")




# Define the ARCHDIR as kernel/arch/x86_64
set(ARCHDIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64)

file(GLOB_RECURSE KERNEL_FILES
#   ${ARCHDIR}/**/*.c 
#   ${ARCHDIR}/**/*.cpp
#   ${ARCHDIR}/IDT/ISR.S
#   ${ARCHDIR}/cpu/context.S
#   src/**/*.c 
#   src/**/*.cpp
    src/kernel/kernel.cpp
    src/tty/tty.cpp
    src/Memory/Memory.cpp
    src/banner/banner.cpp
    src/cxx/init.cpp
    src/sys/handlers.cpp
    src/sys/IOPort.cpp
    src/hardware/pic.cpp
    ${ARCHDIR}/GDT/gdt.cpp
    ${ARCHDIR}/IDT/idt.cpp
    ${ARCHDIR}/IDT/ISR.S
)

# Print the list of found objects
message(STATUS "Found kernel objects: ${KERNEL_FILES} ")

# Add include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSROOT}/usr/include
    ${SYSROOT}/usr/include/cpp
)

set(KERNEL_PHYSICAL_START 0x0000000000400000)
set(KERNEL_VIRTUAL_LOAD_OFFSET 0xFFFFFFFF80000000)

add_compile_definitions(
    KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}
    KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}
)


set( CRTS_FILES crts )

add_library( ${CRTS_FILES} OBJECT
    ${ARCHDIR}/boot/boot.S 
    ${ARCHDIR}/crt/crti.S
    ${ARCHDIR}/crt/crtn.S
)

# Create the atlOS.kernel executable
add_executable(atlOS.kernel  
    ${KERNEL_FILES}
)


target_link_options(atlOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld -nostdlib -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 
)

target_link_options(atlOS.kernel PRIVATE
    "-Wl,--defsym,KERNEL_PHYSICAL_START=${KERNEL_PHYSICAL_START}"
    "-Wl,--defsym,KERNEL_VIRTUAL_LOAD_OFFSET=${KERNEL_VIRTUAL_LOAD_OFFSET}"
)


# Link order and list for the executable
target_link_libraries(atlOS.kernel PRIVATE
    k
    gcc
)

target_link_directories(atlOS.kernel PRIVATE
    ${SYSROOT}/usr/lib
)


# This ensures the boot.o crti.o, crtn.o to be build prior to BARE_METAL target
add_dependencies( atlOS.kernel  ${CRTS_FILES} )

# Ask GCC to get the full path name of crtbegin. and crtend.o
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


set( CMAKE_CXX_LINK_EXECUTABLE 
"<CMAKE_CXX_COMPILER> --sysroot=${SYSROOT} -isystem=/usr/include \
 <LINK_FLAGS> -z max-page-size=0x1000\
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/x86_64/crt/crti.S.obj \
 ${CRTBEGIN_O} \
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/x86_64/boot/boot.S.obj \
 <OBJECTS> \
 -o <TARGET> \
 <LINK_LIBRARIES> \
 ${CRTEND_O} \
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/x86_64/crt/crtn.S.obj" )

# Add a custom command to copy atlOS.kernel to SYSROOT/boot
add_custom_command(
    TARGET atlOS.kernel POST_BUILD
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/atlOS.kernel ${SYSROOT}/boot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Install header files from include/kernel/public to SYSROOT/usr/include/public
install( DIRECTORY include/kernel DESTINATION ${SYSROOT}/usr/include )

# Install header files from include/common to SYSROOT/usr/include/common
install(DIRECTORY include/common DESTINATION ${SYSROOT}/usr/include)