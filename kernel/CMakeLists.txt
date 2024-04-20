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
SET (CMAKE_CXX_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function -std=c++11 -fno-exceptions -fno-rtti -D__is_kernel")
SET (CMAKE_C_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function  -fno-exceptions -fno-rtti -D__is_kernel")
SET (CMAKE_ASM_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function  -fno-exceptions -fno-rtti -D__is_kernel")

# Define the ARCHDIR as kernel/arch/i386
set(ARCHDIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386)

file(GLOB_RECURSE KERNEL_FILES
    ${ARCHDIR}/**/*.c 
    ${ARCHDIR}/**/*.cpp
    kernel/*.cpp
)

# Print the list of found objects
message(STATUS "Found kernel objects: ${KERNEL_FILES} ")

# Add include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSROOT}/usr/include
)

set( BARE_METAL_CRTS app-crts )

add_library( ${BARE_METAL_CRTS} OBJECT
    ${ARCHDIR}/boot/boot.S  # contains symbols analogical to crt0.s
    ${ARCHDIR}/crt/crti.S
    ${ARCHDIR}/crt/crtn.S
)

# Create the ParsOS.kernel executable
add_executable(ParsOS.kernel  
    ${KERNEL_FILES}
)

target_link_options(ParsOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld -nostdlib -ffreestanding  
)

# Link order and list for the executable
target_link_libraries(ParsOS.kernel PRIVATE
    k
    gcc
)

target_link_directories(ParsOS.kernel PRIVATE
    ${SYSROOT}/usr/lib
)

# This ensures the boot.o crti.o, crtn.o to be build prior to BARE_METAL target
add_dependencies( ParsOS.kernel  ${BARE_METAL_CRTS} )

# Ask GCC to get the full path name of crtbegin. and crtend.o
execute_process( COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_O OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtend.o OUTPUT_VARIABLE CRTEND_O OUTPUT_STRIP_TRAILING_WHITESPACE)


# CMAKE_CXX_LINK_EXECUTABLE değişkenini yürütülebilir dosya oluşturma işlemi için ayarla
#set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> --sysroot=${SYSROOT} -isystem=/usr/include -o <TARGET> <LINK_FLAGS> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")
set( CMAKE_CXX_LINK_EXECUTABLE 
"<CMAKE_CXX_COMPILER> --sysroot=${SYSROOT} -isystem=/usr/include \
 <LINK_FLAGS> \
 ${CMAKE_BINARY_DIR}/CMakeFiles/app-crts.dir/arch/i386/crt/crti.S.obj \
 ${ARCHDIR}/crt/crtbegin.o \
 ${CMAKE_BINARY_DIR}/CMakeFiles/app-crts.dir/arch/i386/boot/boot.S.obj \
 <OBJECTS> \
 -o <TARGET> \
 <LINK_LIBRARIES> \
 ${ARCHDIR}/crt/crtend.o \
 ${CMAKE_BINARY_DIR}/CMakeFiles/app-crts.dir/arch/i386/crt/crtn.S.obj" )

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