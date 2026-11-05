#!/usr/bin/env python3
"""Record the animated GIF of the README: the VGA screen from GRUB through the
boot to a few shell commands, through QEMU's monitor ("screendump"). The
commands are typed over the serial port one key at a time; the console shows
them on the screen too.

Usage: scripts/make-demo-gif.py <atlOS.iso> <disk.img> <output.gif>
Needs Python's PIL.
"""

import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

from PIL import Image

COMMANDS = [
    "help",
    "lspci",
    "smbios",
    "ls /disk0/BIN",
    'run /disk0/BIN/ARGS.ELF one "two words"',
    "tasks",
]

BOOT_SLICE_S = 0.02   # the boot runs in slices this long, the VM stopped for each screendump
BOOT_FRAME_MS = 120   # how long each frame of the boot shows
KEY_FRAME_MS = 70     # each typed key
RESULT_MS = 2200      # a command's output, before the next one is typed
LAST_MS = 4000        # the end, before the GIF starts again


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


class Serial:
    def __init__(self, sock):
        self.sock = sock
        self.sock.settimeout(0.05)
        self.data = b""

    def poll(self):
        try:
            chunk = self.sock.recv(65536)
        except socket.timeout:
            return
        if not chunk:
            raise EOFError("QEMU closed the serial port")
        self.data += chunk

    def seen(self, marker):
        """marker arrived since the last call; what came before it is dropped"""
        self.poll()
        index = self.data.find(marker)
        if index < 0:
            return False
        self.data = self.data[index + len(marker):]
        return True


class Recorder:
    def __init__(self, monitor, work):
        self.monitor = monitor
        self.work = work
        self.frames = []  # [image, milliseconds]

    def command(self, line):
        self.monitor.sendall((line + "\n").encode())
        # Its answers are not needed; read them so the socket never fills up
        self.monitor.settimeout(0.01)
        try:
            while self.monitor.recv(65536):
                pass
        except socket.timeout:
            pass

    def grab(self, ms):
        path = os.path.join(self.work, f"{len(self.frames)}.ppm")
        self.monitor.sendall(f"screendump {path}\n".encode())
        for _ in range(100):
            if os.path.exists(path) and os.path.getsize(path) > 0:
                break
            time.sleep(0.02)
        time.sleep(0.05)  # the write may still be going on
        image = Image.open(path).convert("RGB")
        image.load()
        os.remove(path)
        # The same picture again only makes the previous frame longer
        if self.frames and self.frames[-1][0].tobytes() == image.tobytes():
            self.frames[-1][1] += ms
        else:
            self.frames.append([image, ms])

    def save(self, out):
        # Text mode has 16 colors: a small exact palette keeps the file small
        frames = [image.quantize(colors=16, method=Image.MEDIANCUT) for image, _ in self.frames]
        durations = [ms for _, ms in self.frames]
        frames[0].save(out, save_all=True, append_images=frames[1:], duration=durations, loop=0, optimize=True)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    iso, disk, out = sys.argv[1:]
    work = tempfile.mkdtemp()
    shutil.copy(disk, os.path.join(work, "disk.img"))
    serial_port, monitor_port = free_port(), free_port()

    cpu = os.environ.get("QEMU_CPU") or open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "qemu-cpu")).read().strip()
    qemu = subprocess.Popen([
        "qemu-system-x86_64", "-cpu", cpu, "-cdrom", iso, "-m", "512M", "-display", "none", "-no-reboot",
        "-drive", f"file={os.path.join(work, 'disk.img')},format=raw,if=ide,index=0", "-boot", "d",
        "-serial", f"tcp:127.0.0.1:{serial_port},server=on,wait=on",
        "-monitor", f"tcp:127.0.0.1:{monitor_port},server=on,wait=off",
    ], stderr=subprocess.DEVNULL)
    try:
        monitor = connect(monitor_port)
        serial = Serial(connect(serial_port))
        recorder = Recorder(monitor, work)

        # The boot: GRUB, the kernel's messages, the banner, the first prompt.
        # Faster than a screendump, so it runs in short slices with the VM
        # stopped for each picture: the GIF shows it slowed down
        deadline = time.time() + 120
        while not serial.seen(b"atlOS:/> "):
            if time.time() > deadline:
                raise TimeoutError("no prompt")
            recorder.command("stop")
            recorder.grab(BOOT_FRAME_MS)
            recorder.command("cont")
            time.sleep(BOOT_SLICE_S)
        recorder.grab(RESULT_MS)

        for command in COMMANDS:
            for key in command:
                serial.sock.sendall(key.encode())
                time.sleep(0.03)
                recorder.grab(KEY_FRAME_MS)
            serial.sock.sendall(b"\r")
            deadline = time.time() + 30
            while not serial.seen(b"atlOS:/> "):
                if time.time() > deadline:
                    raise TimeoutError(command)
            time.sleep(0.2)
            recorder.grab(RESULT_MS)
        recorder.frames[-1][1] = LAST_MS

        recorder.save(out)
        print(f"{out}: {len(recorder.frames)} frames, {os.path.getsize(out) // 1024} KB")
    finally:
        qemu.kill()
        qemu.wait()
        shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
