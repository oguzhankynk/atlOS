#!/bin/bash

# Paths are relative to this script, so it can be run from anywhere
DIR=$(dirname "$(realpath "$0")")

# Specify the desired FAT format as an argument
# Usage: tools/disk/create_disk_image.sh [12|16]

# Set default format to FAT16
FORMAT_FAT=06
FAT_PARAM=16
SELECTED_FORMAT="FAT16"

# Check the input argument to determine FAT format
if [ "$1" == "16" ]; then
  FORMAT_FAT=06
  FAT_PARAM=16
  SELECTED_FORMAT="FAT16"
  echo "Selected format: FAT16"
elif [ "$1" == "12" ]; then
  FORMAT_FAT=01
  FAT_PARAM=12
  SELECTED_FORMAT="FAT12"
  echo "Selected format: FAT12"
else
  echo "Invalid or no format specified, defaulting to FAT12."
fi

# Output the selected format code
echo "FORMAT_FAT=${FORMAT_FAT}"
echo "FAT_PARAM=${FAT_PARAM}"


# Defines
IMAGE_FILE="$DIR/disk.img"
IMAGE_SIZE_MB=128
PARTITION_COUNT=4
LOOP_DEVICE="/dev/loop0"

# Disk size
IMAGE_SIZE_BYTES=$((IMAGE_SIZE_MB * 1024 * 1024))
IMAGE_SIZE_SECTORS=$((IMAGE_SIZE_BYTES / 512))

# Number of sectors at each partition
PARTITION_SIZE_SECTORS=$((IMAGE_SIZE_SECTORS / PARTITION_COUNT))

# Start and end sector index for Partitions
START_SECTOR=2048
END_SECTORS=()
for i in $(seq 1 $PARTITION_COUNT); do
  END_SECTOR=$(($START_SECTOR + PARTITION_SIZE_SECTORS - 1))
  if [ $i -eq $PARTITION_COUNT ]; then
    END_SECTOR=$((IMAGE_SIZE_SECTORS - 1)) 
  fi
  END_SECTORS+=($END_SECTOR)
  START_SECTOR=$(($END_SECTOR + 1))
done

# printf info
echo "Disk size: $IMAGE_SIZE_MB MB"
echo "The number of total sectors: $IMAGE_SIZE_SECTORS"
echo "The number of sectors that each partitions have: $PARTITION_SIZE_SECTORS "
echo "Partition start-end sector index"
for i in $(seq 1 $PARTITION_COUNT); do
  echo "Partition $i: $((2048 + (i - 1) * PARTITION_SIZE_SECTORS)) - ${END_SECTORS[$((i - 1))]}"
done

# Create an empty disk image
echo "Creating: $IMAGE_FILE ($IMAGE_SIZE_MB MB)"
dd if=/dev/zero of=$IMAGE_FILE bs=1M count=$IMAGE_SIZE_MB

# Attach the disk image to a loop device
echo "Attaching: $IMAGE_FILE -> $LOOP_DEVICE"
sudo losetup $LOOP_DEVICE $IMAGE_FILE

# Create a partition using `fdisk`
(
echo o       # Create a new MBR (Master Boot Record)
for i in $(seq 1 $PARTITION_COUNT); do
  echo n       # Add a new partition
  echo p       # Primary partition

  if [ $i -eq $PARTITION_COUNT ]; then
    :
  else
    echo $i    # Partition number
  fi

  echo $((2048 + (i - 1) * PARTITION_SIZE_SECTORS))  # Start sector
  echo ${END_SECTORS[$((i - 1))]}  # End sector
done
  echo w # Write changes

) | sudo fdisk $LOOP_DEVICE

# Inform the kernel of partition table changes
echo "Updating partition table"
sudo partprobe $LOOP_DEVICE || sudo partx -u $LOOP_DEVICE

# Check the partition table
echo "Verifying partition"
sudo fdisk -l $LOOP_DEVICE

# Set partition type to selected FAT
echo "Changing partition type to $SELECTED_FORMAT($FORMAT_FAT)"
(

for i in $(seq 1 $PARTITION_COUNT); do
  echo t       # Change a partition's system id
 
  echo $i    # Partition number

  echo $FORMAT_FAT  # Format type
  
done

echo w   # Write changes

) | sudo fdisk $LOOP_DEVICE

# Inform the kernel of partition type changes
echo "Updating partition table after type change"
sudo partprobe $LOOP_DEVICE || sudo partx -u $LOOP_DEVICE

# Detach the loop device
echo "Detaching: $LOOP_DEVICE"
sudo losetup -d $LOOP_DEVICE

echo "Process completed: $IMAGE_FILE has been successfully created, partitioned."

echo "Formatting: $SELECTED_FORMAT"
sudo losetup -P $LOOP_DEVICE $IMAGE_FILE
sudo partx -u $LOOP_DEVICE

for i in $(seq 1 $PARTITION_COUNT); do
  sudo mkfs.fat -F $FAT_PARAM ${LOOP_DEVICE}p$i
done

sudo fdisk -l $LOOP_DEVICE
sudo losetup -d $LOOP_DEVICE