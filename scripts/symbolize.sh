#!/bin/sh
# Turn the addresses of a fault report or panic backtrace into function names
# and source lines.
#
# Usage: scripts/symbolize.sh <image> [serial.log]
#   <image> is the ELF file the addresses belong to: the kernel
#   (build/default/kernel/atlOS.kernel) for a kernel panic, the program
#   (build/default/user/fault.elf) for a fault in user mode.
#   Reads the log (or stdin) and resolves every "#<n> 0x<address>" line of a
#   backtrace and every "RIP=0x<address>" of a register dump, e.g.
#     scripts/symbolize.sh build/default/kernel/atlOS.kernel build/selftest/selftest.log
#
# Return addresses point at the instruction after the call, so the line shown
# for #1 and up can be the one after the call.

IMAGE=$1
LOG=${2:--}

if [ -z "$IMAGE" ] || [ ! -f "$IMAGE" ]; then
    echo "usage: $0 <image> [serial.log]" >&2
    exit 2
fi

# The cross binutils if there, else the host's (both read x86_64 ELF)
ADDR2LINE=${ADDR2LINE:-x86_64-elf-addr2line}
if ! command -v "$ADDR2LINE" > /dev/null 2>&1; then
    ADDR2LINE=addr2line
fi

cat "$LOG" | tr -d '\r' | while read -r line; do
    case $line in
        \#[0-9]*" 0x"*)
            frame=${line%% *}
            address=${line#* }
            address=${address%% *}
            echo "$frame $address $("$ADDR2LINE" -f -C -p -e "$IMAGE" "$address")"
            ;;
        RIP=0x*)
            address=${line#RIP=}
            address=${address%% *}
            echo "RIP $address $("$ADDR2LINE" -f -C -p -e "$IMAGE" "$address")"
            ;;
    esac
done
