#!/bin/bash

# File name and size of the image
IMAGE_FILE="VirtualDisk/disk.img"
IMAGE_SIZE_MB=8

# Loop device
LOOP_DEVICE="/dev/loop0"

# Create an empty disk image
echo "Creating: $IMAGE_FILE ($IMAGE_SIZE_MB MB)"
dd if=/dev/zero of=$IMAGE_FILE bs=1M count=$IMAGE_SIZE_MB

# Attach the disk image to a loop device
echo "Attaching: $IMAGE_FILE -> $LOOP_DEVICE"
sudo losetup $LOOP_DEVICE $IMAGE_FILE

# Create a partition using `fdisk`
echo "Creating partition"
(
echo o   # Create a new MBR (Master Boot Record)
echo n   # Add a new partition
echo p   # Primary partition
echo 1   # Partition number 1
echo     # Default start sector
echo     # Default end sector
echo w   # Write changes
) | sudo fdisk $LOOP_DEVICE

# Inform the kernel of partition table changes
echo "Updating partition table"
sudo partprobe $LOOP_DEVICE || sudo partx -u $LOOP_DEVICE

# Check the partition table
echo "Verifying partition"
sudo fdisk -l $LOOP_DEVICE

# Set partition type to FAT12 (0x01)
echo "Changing partition type to FAT12 (0x01)"
(
echo t   # Change a partition's system id
#echo 1   # Partition number 1
echo 01  # FAT12
echo w   # Write changes
) | sudo fdisk $LOOP_DEVICE

# Inform the kernel of partition type changes
echo "Updating partition table after type change"
sudo partprobe $LOOP_DEVICE || sudo partx -u $LOOP_DEVICE

# Detach the loop device
echo "Detaching: $LOOP_DEVICE"
sudo losetup -d $LOOP_DEVICE


echo "Process completed: $IMAGE_FILE has been successfully created, partitioned."

echo "Formatting: FAT12"
sudo losetup -P $LOOP_DEVICE $IMAGE_FILE
sudo partx -u $LOOP_DEVICE
sudo mkfs.fat -F 12 ${LOOP_DEVICE}p1
sudo fdisk -l $LOOP_DEVICE
sudo losetup -d $LOOP_DEVICE