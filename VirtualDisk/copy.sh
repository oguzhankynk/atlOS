#!/bin/bash

SRC_FILE="VirtualDisk/hello.txt"

IMAGE_FILE="VirtualDisk/disk.img"
LOOP_DEVICE="/dev/loop0"

sudo losetup -P $LOOP_DEVICE $IMAGE_FILE

#list partitions
sudo partx -s $LOOP_DEVICE

sudo mount ${LOOP_DEVICE}p1 /mnt

sudo cp $SRC_FILE /mnt/

ls /mnt
sudo umount /mnt
sudo losetup -d $LOOP_DEVICE