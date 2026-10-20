#!/bin/sh
# Turn the addresses of a panic backtrace into function names and source lines.
#
# Usage: scripts/symbolize.sh <atlOS.kernel> [serial.log]
#   Reads the log (or stdin) and resolves every "#<n> 0x<address>" line
#   against the kernel image (built with -g), e.g.
#     scripts/symbolize.sh build/default/kernel/atlOS.kernel build/selftest/selftest.log
#
# Return addresses point at the instruction after the call, so the line shown
# for #1 and up can be the one after the call.

KERNEL=$1
LOG=${2:--}

if [ -z "$KERNEL" ] || [ ! -f "$KERNEL" ]; then
    echo "usage: $0 <atlOS.kernel> [serial.log]" >&2
    exit 2
fi

# The cross binutils if there, else the host's (both read x86_64 ELF)
ADDR2LINE=${ADDR2LINE:-x86_64-elf-addr2line}
if ! command -v "$ADDR2LINE" > /dev/null 2>&1; then
    ADDR2LINE=addr2line
fi

cat "$LOG" | tr -d '\r' | grep -aE '^ *#[0-9]+ 0x[0-9a-fA-F]+' | while read -r frame address; do
    echo "$frame $address $("$ADDR2LINE" -f -C -p -e "$KERNEL" "$address")"
done
