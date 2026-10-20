#!/usr/bin/env python3
"""Take the screenshots of the docs: the VGA screen after boot and during a
shell session, through QEMU's monitor ("screendump"). Commands are typed over
the serial port; the console shows them on the screen too.

Usage: scripts/take-screenshots.py <atlOS.iso> <disk.img> <output dir>
Writes boot.png and shell.png (needs Python's PIL).
"""

import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

from PIL import Image


def free_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def connect(port):
    for _ in range(100):
        try:
            return socket.create_connection(("127.0.0.1", port))
        except OSError:
            time.sleep(0.1)
    raise RuntimeError(f"no connection on port {port}")


def wait_for(sock, marker, timeout=30):
    data = b""
    deadline = time.time() + timeout
    sock.settimeout(0.5)
    while marker not in data:
        if time.time() > deadline:
            raise TimeoutError(marker)
        try:
            data += sock.recv(4096)
        except socket.timeout:
            pass
    return data


def screendump(monitor, path):
    monitor.sendall(f"screendump {path}\n".encode())
    for _ in range(50):
        if os.path.exists(path) and os.path.getsize(path) > 0:
            time.sleep(0.3)
            return
        time.sleep(0.1)
    raise RuntimeError("screendump did not write " + path)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    iso, disk, out = sys.argv[1:]
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp()
    shutil.copy(disk, os.path.join(work, "disk.img"))
    serial_port, monitor_port = free_port(), free_port()

    qemu = subprocess.Popen([
        "qemu-system-x86_64", "-cdrom", iso, "-m", "512M", "-display", "none", "-no-reboot",
        "-drive", f"file={os.path.join(work, 'disk.img')},format=raw,if=ide,index=0", "-boot", "d",
        "-serial", f"tcp:127.0.0.1:{serial_port},server=on,wait=on",
        "-monitor", f"tcp:127.0.0.1:{monitor_port},server=on,wait=off",
    ], stderr=subprocess.DEVNULL)
    try:
        serial = connect(serial_port)
        monitor = connect(monitor_port)
        wait_for(serial, b"atlOS:/> ", timeout=60)
        time.sleep(0.5)
        screendump(monitor, os.path.join(work, "boot.ppm"))

        for command in ["clear", "ls /disk0/BIN", "run /disk0/BIN/HELLO.ELF", "tasks", "vmmap"]:
            serial.sendall((command + "\r").encode())
            wait_for(serial, b"exited with" if command.startswith("run") else b"atlOS:/> ")
            time.sleep(0.3)
        screendump(monitor, os.path.join(work, "shell.ppm"))

        for name in ("boot", "shell"):
            Image.open(os.path.join(work, name + ".ppm")).save(os.path.join(out, name + ".png"), optimize=True)
            print(os.path.join(out, name + ".png"))
    finally:
        qemu.kill()
        qemu.wait()
        shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
