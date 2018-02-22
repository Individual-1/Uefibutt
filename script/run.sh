#!/bin/sh

BASE_BUILD=$WORKSPACE/Build/Uefibutt/RELEASE_GCC5/X64

if [ ! -d $BASE_BUILD ]; then
    echo "$BASE_BUILD is not a valid directory" >&2
    exit 1
fi

qemu-system-x86_64 -L $OVMF_DIR -bios $OVMF_DIR/OVMF-pure-efi.fd -cdrom $BASE_BUILD/Uefibutt.img
