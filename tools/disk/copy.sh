#!/bin/bash

# Paths are relative to this script, so it can be run from anywhere
DIR=$(dirname "$(realpath "$0")")

SRC_FILE="$DIR/MY_HELLO.txt"

IMAGE_FILE="$DIR/disk.img"
LOOP_DEVICE="/dev/loop0"

sudo losetup -P $LOOP_DEVICE $IMAGE_FILE

#list partitions
sudo partx -s $LOOP_DEVICE

sudo mount -o rw ${LOOP_DEVICE}p1 /mnt

sudo cp $SRC_FILE /mnt/

ls /mnt
sudo umount /mnt
sudo losetup -d $LOOP_DEVICE