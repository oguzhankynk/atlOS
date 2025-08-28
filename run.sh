 #!/bin/sh
set -e

for arg in "$@"; do
    if [ "$arg" == "clean" ]; then
        ./clean.sh
        break
    fi
done

for arg in "$@"; do
    if [ "$arg" == "build-libc" ]; then
        ./libcCMake.sh
        break
    fi
done

for arg in "$@"; do
    if [ "$arg" == "build-kernel" ]; then
        ./kernelCMake.sh
        break
    fi
done

for arg in "$@"; do
    if [ "$arg" == "build-all" ]; then
        ./libcCMake.sh
        ./kernelCMake.sh
        break
    fi
done

for arg in "$@"; do
    if [ "$arg" == "iso" ]; then
        ./iso.sh
        break
    fi
done

for arg in "$@"; do
    if [ "$arg" == "run" ]; then
        ./qemu.sh 
        break
    fi
done