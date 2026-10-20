# FAT test disk

Scripts to create a raw disk image with FAT partitions for the (not yet wired
up) ATA and FAT16 drivers. The image is generated, never committed:
`tools/disk/disk.img` is in `.gitignore`.

## Create the image

```sh
tools/disk/create_disk_image.sh 16   # FAT16 (also the default)
tools/disk/create_disk_image.sh 12   # FAT12
```

This writes `tools/disk/disk.img`:

- 128 MB, MBR partition table
- four primary partitions of about 32 MB each, the first one starting at
  sector 2048
- every partition formatted with `mkfs.fat -F 16` (or `-F 12`), partition
  type `0x06` (FAT16) or `0x01` (FAT12)

## Put a file on it

```sh
tools/disk/copy.sh
```

copies `tools/disk/MY_HELLO.txt` to the first partition (mounted on `/mnt`
for the copy).

## Requirements

Both scripts use `sudo` and the loop device `/dev/loop0`, which must be free
(`losetup -a` lists the ones in use). They need `losetup`, `partx` or
`partprobe`, `fdisk`, `mount` and `mkfs.fat`; on Debian/Ubuntu:

```sh
sudo apt install util-linux fdisk parted dosfstools
```

## Use it in QEMU

The `run`/`debug` targets do not attach the disk yet, since the kernel does
not detect disks. To try it by hand:

```sh
qemu-system-x86_64 -cdrom build/default/atlOS.iso -m 512M -serial stdio \
    -drive file=tools/disk/disk.img,format=raw,if=ide
```
