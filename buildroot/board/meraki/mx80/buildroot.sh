#!/bin/sh

# create a symlink to buildroot-latest
if [ ! -e /src/buildroot-latest ]; then
    ln -s $(basename $(ls -1d /src/buildroot-20*)) buildroot-latest
fi

# symlink the overlay into the buildroot tree
if [ ! -e /src/buildroot-latest/board/meraki ]; then
    ln -s /src/meraki-builder/buildroot/board/meraki /src/buildroot-latest/board/meraki
fi

# symlink the buildroot config to .config
if [ ! -e /src/buildroot-latest/.config ]; then
    cd /src/buildroot-latest && ln -s board/meraki/mx80/buildroot-config .config
fi

# now make buildroot
cd /src/buildroot-latest && make

if [ $? -eq 0 ] && [ -f /src/buildroot-latest/output/images/ubi_image.bin ]; then
    echo "Your firmware is ready in src/buildroot-latest/output/images/ubi_image.bin"
fi
