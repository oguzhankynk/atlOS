cmake_minimum_required(VERSION 3.22.1)

# Include the ParsOS platform-specific settings
include(${CMAKE_MODULE_PATH}/Platform/ParsOS.cmake)

# Check if SYSROOT is defined
if(NOT DEFINED SYSROOT)
    # Print error message and stop the configuration process
    message(FATAL_ERROR "SYSROOT is not defined. Please define SYSROOT to specify the target root directory.")
endif()

# Define the project name
project(ParsOS_libk)

# Define C++ compiler flags for the project
SET (CMAKE_CXX_FLAGS "--sysroot=${SYSROOT} -isystem=/usr/include \
-std=c++11  -ffreestanding -Wall -Wextra  -Wall -Wno-unused-variable -Wno-unused-function\
-D__is_libc -D__is_libk ")

# Recursively find all .cpp source files in stdio, stdlib, and string directories
file(GLOB_RECURSE SOURCES
    stdio/*.cpp
    stdlib/*.cpp
    string/*.cpp
)

# Print the list of found sources
message(STATUS "Found sources: ${SOURCES}")

# Add include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create a static library 'k' using the found sources
add_library(k STATIC  ${SOURCES})

SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> rcs  <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> rcs  <TARGET> <LINK_FLAGS> <OBJECTS>")

# Copy the `k` static library to the target destination directories
install(TARGETS k
    LIBRARY DESTINATION ${SYSROOT}/usr/lib
    ARCHIVE DESTINATION ${SYSROOT}/usr/lib
    )

# Copy the headers to the target destination directories
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION ${SYSROOT}/usr)
   
