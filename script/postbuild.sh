#!/bin/sh

BASE_BUILD=$WORKSPACE/Build/Uefibutt/RELEASE_GCC5/X64

if [ ! -d $BASE_BUILD ]; then
    echo "$BASE_BUILD is not a valid directory" >&2
    exit 1
fi

echo "Creating image file"
$WORKSPACE/Uefibutt/create_img.sh $BASE_BUILD/Uefibutt.img
echo "Creating iso file"
$WORKSPACE/Uefibutt/create_iso.sh $BASE_BUILD Uefibutt.iso Uefibutt.img
