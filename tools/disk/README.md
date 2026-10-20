# Test disk

Files for the test disk: a raw image with an MBR and one FAT16 partition,
which the self tests read as `/disk0`. The image is generated, never
committed.

```sh
cmake --build --preset default --target disk-image   # build/default/disk.img
scripts/make-disk-image.sh some/where/disk.img       # or directly
```

`scripts/make-disk-image.sh` needs no root: it partitions the file with
`sfdisk`, formats the partition in place with `mkfs.fat --offset` and copies
the files with mtools (`mcopy -i disk.img@@1048576`). The image:

- 32 MB (`DISK_SIZE_MB` changes it), one primary partition of type `0x06`
  from sector 2048 (1 MB) to the end
- FAT16, volume label `ATLOS`
- `files/HELLO.TXT`, `files/DOCS/README.TXT`, and a generated `BIG.TXT`
  (2000 lines `line 00001` .. `line 02000`, 22000 bytes, many clusters)

The self tests (`kernel/tests/selftest.cpp`, *ATA disk*) compare the contents
with what they expect, then write `KERNEL/NOTE.TXT` and `KERNEL/DATA.TXT` to
a copy of the image; `scripts/check-test-disk.sh` checks that copy on the host
with `fsck.fat` and mtools. Change them together.

To look inside:

```sh
mdir -i build/default/disk.img@@1048576 ::/
mtype -i build/default/disk.img@@1048576 ::/HELLO.TXT
```

Requirements on Debian/Ubuntu: `sudo apt install fdisk dosfstools mtools`.
