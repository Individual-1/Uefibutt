#!/bin/sh

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 OUTPUT_DIR EFI_ISO_NAME EFI_IMG_NAME" >&2
    exit 1
fi

if [ ! -e "$1/$3" ]; then
    echo "$1/$3 does not exist" >&2
    exit 1
fi

# Create iso file
dir="$1"
target="$2"
img="$3"

rm -rf $dir/iso
mkdir $dir/iso
cp $dir/$img $dir/iso
xorriso -as mkisofs -R -f -e $img -no-emul-boot -o $dir/$target $dir/iso
rm -rf $dir/iso
