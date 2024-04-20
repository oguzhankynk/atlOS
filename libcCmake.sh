#!/bin/bash


SCRIPT_DIR=$(dirname "$(realpath "$0")")

# SYSROOT
SYSROOT="$SCRIPT_DIR/sysroot"
mkdir "$SYSROOT"
mkdir "$SYSROOT/boot"
mkdir "$SYSROOT/usr"

PROJECT_DIR="$SCRIPT_DIR/libc"


BUILD_DIR="$PROJECT_DIR/build"
echo "Cleaning and creating build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"


cd "$BUILD_DIR" || exit


echo "Running CMake..."
cmake "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/CMakeModule/ParsOS.toolchain.cmake" \
    -DCMAKE_MODULE_PATH="${SCRIPT_DIR}/CMakeModule" \
    -DSYSROOT="${SYSROOT}" \
    -B"${PROJECT_DIR}/build"


if [ $? -ne 0 ]; then
  echo "CMake configuration failed. Exiting..."
  exit 1
fi


echo "Compiling the project..."
make VERBOSE=1


if [ $? -ne 0 ]; then
  echo "Compilation failed. Exiting..."
  exit 1
fi

echo "Running Make Install..."
make install

if [ $? -ne 0 ]; then
  echo "Make install failed. Exiting..."
  exit 1
fi

echo "Compilation completed successfully."