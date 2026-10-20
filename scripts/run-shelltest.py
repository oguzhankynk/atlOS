#!/usr/bin/env python3
"""Boot the normal kernel and drive its shell over the serial port.

QEMU's COM1 is a TCP socket; the script types commands (with the same bytes
a terminal sends: '\\r' for Enter, DEL for Backspace, ESC [ A for Up) and
checks what comes back. Used by the "shelltest" target.

Usage: scripts/run-shelltest.py <atlOS.iso> <disk.img> [serial.log]
Exit code: 0 = every check passed, 1 = a check failed, 2 = boot/timeout problem.
"""

import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

PROMPT = b"\natlOS:"  # a prompt starts a line: "atlOS:/disk0> "
TIMEOUT = float(os.environ.get("SHELLTEST_TIMEOUT", "30"))


class Serial:
    def __init__(self, sock, log):
        self.sock = sock
        self.log = log
        self.buffer = b""

    def read_until(self, marker, timeout=TIMEOUT):
        deadline = time.time() + timeout
        while marker not in self.buffer:
            left = deadline - time.time()
            if left <= 0:
                raise TimeoutError(marker)
            self.sock.settimeout(left)
            try:
                data = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not data:
                raise EOFError("QEMU closed the serial port")
            self.log.write(data)
            self.log.flush()
            self.buffer += data
        index = self.buffer.index(marker) + len(marker)
        text, self.buffer = self.buffer[:index], self.buffer[index:]
        return text.decode("latin-1").replace("\r", "")

    def read_prompt(self, timeout=TIMEOUT):
        """Everything up to and including the next prompt."""
        text = self.read_until(PROMPT, timeout)
        return text + self.read_until(b"> ", timeout)

    def command(self, keys):
        """Type keys (Enter included by the caller) and return the output up to the next prompt."""
        self.sock.sendall(keys.encode("latin-1"))
        return self.read_prompt()


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    iso, disk = sys.argv[1], sys.argv[2]
    log_path = sys.argv[3] if len(sys.argv) > 3 else "shelltest.log"

    # The shell writes to the disk: use a copy
    work = tempfile.mkdtemp()
    disk_copy = os.path.join(work, "disk.img")
    shutil.copy(disk, disk_copy)

    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    qemu = subprocess.Popen([
        "qemu-system-x86_64", "-cdrom", iso, "-m", "512M", "-display", "none", "-no-reboot",
        "-drive", f"file={disk_copy},format=raw,if=ide,index=0", "-boot", "d",
        "-serial", f"tcp:127.0.0.1:{port},server=on,wait=on",
    ])

    failed = 0
    passed = 0

    def check(ok, what, output=""):
        nonlocal failed, passed
        if ok:
            passed += 1
            print(f"  ok:      {what}")
        else:
            failed += 1
            print(f"  wrong:   {what}")
            if output:
                print("    " + output.replace("\n", "\n    "))

    try:
        sock = None
        for _ in range(100):
            try:
                sock = socket.create_connection(("127.0.0.1", port))
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            print("SHELLTEST: could not connect to QEMU's serial port")
            return 2

        with sock, open(log_path, "wb") as log:
            serial = Serial(sock, log)
            boot = serial.read_prompt(timeout=60)
            check("mounted on /disk0" in boot, "boot mounts the test disk", boot[-300:])

            out = serial.command("help\r")
            check("md addr [len]" in out and "vmmap" in out, "help lists the commands", out)

            out = serial.command("ls /disk0\r")
            check("HELLO.TXT" in out and "DOCS/" in out and "22000" in out, "ls /disk0", out)

            out = serial.command("cat /disk0/HELLO.TXT\r")
            check("Hello from the atlOS disk!" in out, "cat a file", out)

            serial.command("cd /disk0/DOCS\r")
            out = serial.command("pwd\r")
            check("/disk0/DOCS\n" in out, "cd and pwd", out)
            out = serial.command("cat README.TXT\r")
            check("a file in a subdirectory" in out, "relative path", out)

            serial.command("cd ..\r")
            serial.command("mkdir SHELL\r")
            serial.command("echo hello from the shell > SHELL/A.TXT\r")
            serial.command("echo second line >> SHELL/A.TXT\r")
            out = serial.command("cat SHELL/A.TXT\r")
            check("hello from the shell\nsecond line\n" in out, "echo > and >>, cat", out)

            # Line editing: DEL as backspace, history with the up arrow, Ctrl+C
            out = serial.command("pwdx\x7f\r")
            check("/disk0\n" in out, "backspace (DEL)", out)
            out = serial.command("\x1b[A\r")
            check("/disk0\n" in out, "up arrow repeats the last command", out)
            out = serial.command("rm SHELL/A.TXT\x03")
            out += serial.command("ls SHELL\r")
            check("^C" in out and "A.TXT" in out, "Ctrl+C drops the line", out)

            serial.command("rm SHELL/A.TXT\r")
            serial.command("rmdir SHELL\r")
            out = serial.command("ls\r")
            check("SHELL/" not in out, "rm and rmdir", out)

            # Memory: write into low memory (identity mapped, unused by the
            # kernel: the PMM never hands out the first 2 MB), read it back
            serial.command("mw 0x7000 0x41\r")
            serial.command("mw 0x7008 0x1122334455667788 8\r")
            out = serial.command("md 0x7000 16\r")
            check("0000000000007000  41" in out and "88 77 66 55 44 33 22 11" in out, "mw and md", out)
            out = serial.command("md 0xb8000 16\r")
            check("|" in out and "00000000000b8000" in out, "md of the VGA text buffer", out)
            out = serial.command("md 0xdead0000\r")
            check("not mapped" in out, "md refuses unmapped memory", out)
            out = serial.command("mem\r")
            check("physical:" in out and "heap:" in out and ".text" in out, "mem", out)
            out = serial.command("vmmap\r")
            check("ffffffff80000000" in out and "2M" in out, "vmmap", out)

            out = serial.command("tasks\r")
            check("kernel" in out and "running" in out and "1 task(s)" in out, "tasks", out)
            out = serial.command("uptime\r")
            check("up " in out, "uptime", out)
            out = serial.command("mount\r")
            check("/disk0" in out and "fat16" in out, "mount", out)
            out = serial.command("nonsense\r")
            check("unknown command" in out, "unknown command", out)

            # The end: a page fault on purpose, reported like any other
            serial.sock.sendall(b"fault\r")
            out = serial.read_until(b"KERNEL PANIC", timeout=10)
            check("EXCEPTION 14: Page fault" in out and "dead0000" in out, "fault: page fault report", out)
    except (TimeoutError, EOFError) as error:
        print(f"SHELLTEST: no answer ({error}), see {log_path}")
        return 2
    finally:
        qemu.kill()
        qemu.wait()
        shutil.rmtree(work, ignore_errors=True)

    print(f"SHELLTEST {'PASSED' if not failed else 'FAILED'}: {passed} ok, {failed} wrong")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
