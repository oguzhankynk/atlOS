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
SET (CMAKE_CXX_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function -std=c++11 -fno-exceptions -fno-rtti -D__is_kernel")
SET (CMAKE_C_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function  -fno-exceptions -fno-rtti -D__is_kernel")
SET (CMAKE_ASM_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include  -ffreestanding -Wall -Wno-unused-variable -Wno-unused-function  -fno-exceptions -fno-rtti -D__is_kernel")

# Define the ARCHDIR as kernel/arch/i386
set(ARCHDIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386)

file(GLOB_RECURSE KERNEL_FILES
    ${ARCHDIR}/**/*.c 
    ${ARCHDIR}/**/*.cpp
    ${ARCHDIR}/IDT/ISR.S
    ${ARCHDIR}/cpu/context.S
    src/**/*.c 
    src/**/*.cpp
)

# Print the list of found objects
message(STATUS "Found kernel objects: ${KERNEL_FILES} ")

# Add include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSROOT}/usr/include
    ${SYSROOT}/usr/include/cpp
)

set( CRTS_FILES crts )

add_library( ${CRTS_FILES} OBJECT
    ${ARCHDIR}/boot/boot.S  # contains symbols analogical to crt0.s
    ${ARCHDIR}/crt/crti.S
    ${ARCHDIR}/crt/crtn.S
)

# Create the atlOS.kernel executable
add_executable(atlOS.kernel  
    ${KERNEL_FILES}
)

target_link_options(atlOS.kernel PRIVATE
    -T${ARCHDIR}/linker.ld -nostdlib -ffreestanding  
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
execute_process( COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_O OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtend.o OUTPUT_VARIABLE CRTEND_O OUTPUT_STRIP_TRAILING_WHITESPACE)

# CMAKE_CXX_LINK_EXECUTABLE değişkenini yürütülebilir dosya oluşturma işlemi için ayarla
#set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> --sysroot=${SYSROOT} -isystem=/usr/include -o <TARGET> <LINK_FLAGS> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")
set( CMAKE_CXX_LINK_EXECUTABLE 
"<CMAKE_CXX_COMPILER> --sysroot=${SYSROOT} -isystem=/usr/include \
 <LINK_FLAGS> \
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/i386/crt/crti.S.obj \
 ${CRTBEGIN_O} \
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/i386/boot/boot.S.obj \
 <OBJECTS> \
 -o <TARGET> \
 <LINK_LIBRARIES> \
 ${CRTEND_O} \
 ${CMAKE_BINARY_DIR}/CMakeFiles/${CRTS_FILES}.dir/arch/i386/crt/crtn.S.obj" )

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