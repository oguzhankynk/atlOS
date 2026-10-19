cmake_minimum_required(VERSION 3.22)

project(atlOS C CXX ASM)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "atlOS")
    message(FATAL_ERROR "Configure with the cross toolchain: cmake --preset default")
endif()

# Run the in-kernel self tests instead of the demo threads (preset "selftest")
option(ATLOS_SELFTEST "Build the kernel in self test mode" OFF)

add_subdirectory(libc)
add_subdirectory(kernel)

# ------------------------------------------------------------
# Bootable ISO
# ------------------------------------------------------------

find_program(GRUB_MKRESCUE NAMES grub2-mkrescue grub-mkrescue)
find_program(QEMU qemu-system-x86_64)

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
# QEMU
# ------------------------------------------------------------

if(GRUB_MKRESCUE AND QEMU)
    # Serial output on the terminal; Ctrl-A X quits
    add_custom_target(run
        COMMAND ${QEMU} -cdrom ${ATLOS_ISO} -m 512M -nographic -monitor none -serial stdio -no-shutdown
        DEPENDS iso
        USES_TERMINAL
        VERBATIM
    )

    # Waits for gdb on :1234, serial output on telnet localhost 4321
    add_custom_target(debug
        COMMAND ${QEMU} -cdrom ${ATLOS_ISO} -m 1024M -no-shutdown -serial telnet:localhost:4321,server,nowait -S -s
        DEPENDS iso
        USES_TERMINAL
        VERBATIM
    )

    if(ATLOS_SELFTEST)
        add_custom_target(selftest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-selftest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/selftest.log
            DEPENDS iso
            USES_TERMINAL
            VERBATIM
        )

        enable_testing()
        add_test(NAME selftest
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-selftest.sh ${ATLOS_ISO} ${CMAKE_BINARY_DIR}/selftest.log)
    endif()
elseif(NOT QEMU)
    message(WARNING "qemu-system-x86_64 not found: the run, debug and selftest targets are not available")
endif()

# ------------------------------------------------------------
# API documentation
# ------------------------------------------------------------

find_program(DOXYGEN doxygen)
if(DOXYGEN)
    add_custom_target(docs
        COMMAND ${DOXYGEN} docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API docs in build/docs"
        VERBATIM
    )
endif()
