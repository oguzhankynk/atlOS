cmake_minimum_required(VERSION 3.22)

# The one place the version is written; see docs/releasing.md
project(atlOS VERSION 0.1.0 LANGUAGES C CXX ASM)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "atlOS")
    message(FATAL_ERROR "Configure with the cross toolchain: cmake --preset default")
endif()

# Run the in-kernel self tests instead of the demo threads (preset "selftest")
option(ATLOS_SELFTEST "Build the kernel in self test mode" OFF)

# Fault on purpose after boot to test the fault report (preset "crashtest")
option(ATLOS_CRASHTEST "Build a kernel that page faults on purpose" OFF)

# I/O port that receives the exit status when the kernel halts (Core::haltWithStatus),
# so a test machine can end the run with it. Empty: nothing is written.
set(ATLOS_DEBUG_EXIT_PORT "" CACHE STRING "I/O port for the exit status at halt, empty for none")

# No MMX/SSE/AVX anywhere in the kernel image (kernel and libc): boot does not
# enable SSE in CR4, and neither the context switch nor the ISRs save SIMD state.
# kernel/CMakeLists.txt checks the linked image for SIMD instructions.
set(ATLOS_NO_SIMD_FLAGS -mno-mmx -mno-sse -mno-sse2)

add_subdirectory(libc)
add_subdirectory(kernel)

# ------------------------------------------------------------
# Bootable ISO
# ------------------------------------------------------------

find_program(GRUB_MKRESCUE NAMES grub2-mkrescue grub-mkrescue)
find_program(QEMU qemu-system-x86_64)
find_program(SFDISK sfdisk PATHS /usr/sbin /sbin)
find_program(MKFS_FAT mkfs.fat PATHS /usr/sbin /sbin)
find_program(MCOPY mcopy)

set(ATLOS_ISO_DIR ${CMAKE_BINARY_DIR}/isodir)
set(ATLOS_ISO ${CMAKE_BINARY_DIR}/atlOS.iso)

if(GRUB_MKRESCUE)
    # The ISO is written next to isodir, never inside it
    add_custom_command(
        OUTPUT ${ATLOS_ISO}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${ATLOS_ISO_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ATLOS_ISO_DIR}/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:atlOS.kernel> ${ATLOS_ISO_DIR}/boot/atlOS.kernel
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/grub/grub.cfg ${ATLOS_ISO_DIR}/boot/grub/grub.cfg
        COMMAND ${GRUB_MKRESCUE} -o ${ATLOS_ISO} ${ATLOS_ISO_DIR}
        DEPENDS atlOS.kernel ${CMAKE_SOURCE_DIR}/tools/grub/grub.cfg
        COMMENT "Creating atlOS.iso"
        VERBATIM
    )
    add_custom_target(iso ALL DEPENDS ${ATLOS_ISO})
else()
    message(WARNING "grub-mkrescue not found: the iso, run, debug and selftest targets are not available")
endif()

# ------------------------------------------------------------
# Test disk: MBR + one FAT16 partition, see scripts/make-disk-image.sh
# ------------------------------------------------------------

set(ATLOS_DISK ${CMAKE_BINARY_DIR}/disk.img)
set(ATLOS_DISK_ARGS "")
set(ATLOS_DISK_TARGET "")
set(ATLOS_SELFTEST_DISK "")

if(SFDISK AND MKFS_FAT AND MCOPY)
    file(GLOB_RECURSE ATLOS_DISK_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tools/disk/files/*)
    add_custom_command(
        OUTPUT ${ATLOS_DISK}
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/make-disk-image.sh ${ATLOS_DISK}
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/make-disk-image.sh ${ATLOS_DISK_FILES}
        COMMENT "Creating disk.img"
        VERBATIM
    )
    add_custom_target(disk-image DEPENDS ${ATLOS_DISK})

    # Primary master; boot from the CD (the disk has an MBR but no boot code)
    set(ATLOS_DISK_ARGS -drive file=${ATLOS_DISK},format=raw,if=ide,index=0 -boot d)
    set(ATLOS_DISK_TARGET disk-image)
    set(ATLOS_SELFTEST_DISK ${ATLOS_DISK})
else()
    message(WARNING "sfdisk, mkfs.fat or mtools not found: no test disk (the disk self tests are skipped)")
endif()

# ------------------------------------------------------------
# QEMU
# ------------------------------------------------------------

if(GRUB_MKRESCUE AND QEMU)
    # Serial output on the terminal; Ctrl-C quits
    add_custom_target(run
        COMMAND ${QEMU} -cdrom ${ATLOS_ISO} ${ATLOS_DISK_ARGS} -m 512M -nographic -monitor none -serial stdio -no-shutdown
        DEPENDS iso ${ATLOS_DISK_TARGET}
        USES_TERMINAL
        VERBATIM
    )

    # Waits for gdb on :1234, serial output on telnet localhost 4321
    add_custom_target(debug
        COMMAND ${QEMU} -cdrom ${ATLOS_ISO} ${ATLOS_DISK_ARGS} -m 1024M -no-shutdown -serial telnet:localhost:4321,server,nowait -S -s
        DEPENDS iso ${ATLOS_DISK_TARGET}
        USES_TERMINAL
        VERBATIM
    )

    if(ATLOS_SELFTEST)
        add_custom_target(selftest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-selftest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/selftest.log ${ATLOS_SELFTEST_DISK}
            DEPENDS iso ${ATLOS_DISK_TARGET}
            USES_TERMINAL
            VERBATIM
        )

        enable_testing()
        add_test(NAME selftest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-selftest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/selftest.log ${ATLOS_SELFTEST_DISK})
    endif()

    if(ATLOS_CRASHTEST)
        add_custom_target(crashtest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-crashtest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/crashtest.log $<TARGET_FILE:atlOS.kernel>
            DEPENDS iso
            USES_TERMINAL
            VERBATIM
        )

        enable_testing()
        add_test(NAME crashtest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-crashtest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/crashtest.log $<TARGET_FILE:atlOS.kernel>)
    endif()
elseif(NOT QEMU)
    message(WARNING "qemu-system-x86_64 not found: the run, debug and selftest targets are not available")
endif()

# ------------------------------------------------------------
# Code style (docs/style.md)
# ------------------------------------------------------------

find_program(CLANG_FORMAT NAMES clang-format clang-format-14)
find_program(RUN_CLANG_TIDY NAMES run-clang-tidy run-clang-tidy-14)

file(GLOB_RECURSE ATLOS_FORMAT_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/kernel/*.cpp ${CMAKE_SOURCE_DIR}/kernel/*.hpp ${CMAKE_SOURCE_DIR}/kernel/*.h
    ${CMAKE_SOURCE_DIR}/libc/*.cpp ${CMAKE_SOURCE_DIR}/libc/*.h
)

if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ATLOS_FORMAT_FILES}
        COMMENT "Formatting sources with clang-format"
        VERBATIM
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ATLOS_FORMAT_FILES}
        COMMENT "Checking source format"
        VERBATIM
    )
endif()

if(RUN_CLANG_TIDY)
    # Kernel sources only; libc and liballoc are not held to the kernel rules
    add_custom_target(tidy
        COMMAND ${RUN_CLANG_TIDY} -p ${CMAKE_BINARY_DIR} -quiet
            -extra-arg=-Wno-unknown-warning-option
            "^(?!.*(/liballoc/|/libc/)).*/kernel/.*\\.cpp$"
        COMMENT "Running clang-tidy"
        VERBATIM
    )
endif()

# ------------------------------------------------------------
# API documentation
# ------------------------------------------------------------

find_program(DOXYGEN doxygen)
if(DOXYGEN)
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E env ATLOS_VERSION=${PROJECT_VERSION} ${DOXYGEN} docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API docs in build/docs"
        VERBATIM
    )
endif()
