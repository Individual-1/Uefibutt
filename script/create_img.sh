#!/bin/sh

UEFIBUTT=$WORKSPACE/Build/Uefibutt/RELEASE_GCC5/X64

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 OUTPUT_BASE_NAME" >&2
    exit 1
fi

if [ ! -e $UEFIBUTT/Uefibutt.efi ]; then
    echo "Target $UEFIBUTT/Uefibutt.efi does not exist" >&2
    exit 1
fi

# Create img file
target="$1"

dd if=/dev/zero of=$target bs=1M count=128
mkfs.vfat -F 32 $target
mmd -i $target ::/EFI
mmd -i $target ::/EFI/BOOT
mmd -i $target ::/TEST
mcopy -i $target $UEFIBUTT/Uefibutt.efi ::/EFI/BOOT/BOOTX64.EFI

mcopy -v -i $target $WORKSPACE/Uefibutt/extras/* ::/TEST
#for file in $WORKSPACE/Uefibutt/extras/*; do 
#    mcopy -v -i $target $file ::/
#done

