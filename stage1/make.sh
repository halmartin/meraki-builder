#!/usr/bin/env bash
# Hal Martin (C) 2020
# https://github.com/halmartin/meraki-builder
# Script to generate flashable image for NOR flash on Cisco Meraki MS220/320/22/42 switches
# This *will not* work for the stock firmware!!

function sizekernel (){
    if [ -f vmlinux.bin ]; then
        KERNEL_PATH=vmlinux.bin
    else
        if [ -f bin/boot1-kernel ]; then
            KERNEL_PATH=bin/boot1-kernel
        elif [ -f boot1-kernel ]; then
            KERNEL_PATH=boot1-kernel
        fi
    fi
    echo -n $(wc -c $KERNEL_PATH | awk '{print $1}')
}

# create boot1-header
HEADER_PATH=bin/boot1-header
# MIPS magic
printf '\x53\x50\x49\x4d' > $HEADER_PATH
# load address: 0x80100000
echo 00000000 00 00 10 80 | xxd -r >> $HEADER_PATH
LENGTH_HEX=$(printf '%x' $(sizekernel))

if [ ${#LENGTH_HEX} -eq 6 ]; then
  LENGTH_HEX="00${LENGTH_HEX}"
fi
echo 00000000 $(echo $LENGTH_HEX | cut -b7-8) $(echo $LENGTH_HEX | cut -b5-6) $(echo $LENGTH_HEX | cut -b3-4) $(echo $LENGTH_HEX | cut -b0-2) | xxd -r >> $HEADER_PATH
# get the entry point of the kernel
if [ -f vmlinux ]; then
    # custom kernel, get the entry point from ELF header
    ENTRY=$(readelf -h vmlinux | grep "Entry point" | awk '{print $4}' | cut -b3-10)
    echo "Got entry point 0x$ENTRY from vmlinux ELF header"
else
    # stock kernel, so parse the entry point from the boot1 header
    ENTRY=$(dd if=boot1 bs=4 skip=3 count=1 | hexdump | awk '{print $3,$2}' | tr -d ' ' | tr -d '\n')
    echo "Got entry point 0x$ENTRY from boot1 header"
fi
echo 00000000 $(echo $ENTRY | cut -b7-8) $(echo $ENTRY | cut -b5-6) $(echo $ENTRY | cut -b3-4) $(echo $ENTRY | cut -b0-2) | xxd -r >> $HEADER_PATH
# fill the remaining 16 bytes of the header with 0s
dd if=/dev/zero of=$HEADER_PATH bs=16 seek=1 count=1 conv=notrunc

if [ -f vmlinux.bin ]; then
    echo "Using vmlinux.bin"
    cat $HEADER_PATH vmlinux.bin > bin/boot1-new
else
    echo "Using boot1-kernel"
    cat $HEADER_PATH bin/boot1-kernel > bin/boot1-new
fi

# CRC is over the header + kernel
# disabled CRC check in redboot, don't need to populate
#CRC=$(crc_32 bin/boot1-new | awk '{print $1}')
#printf "\x$(echo $CRC | cut -b7-8)\x$(echo $CRC | cut -b5-6)\x$(echo $CRC | cut -b3-4)\x$(echo $CRC | cut -b0-2)" > bin/crc.bin
#echo "Patching boot1-new with CRC: $CRC"
#dd if=bin/crc.bin of=bin/boot1-new bs=1 seek=16 conv=notrunc

SIZE_BOOT1=$(wc -c bin/boot1-new | awk '{print $1}')
NEEDED_WHITESPACE=$((0x4c0000 - $SIZE_BOOT1))
echo "Padding boot1-new with $NEEDED_WHITESPACE bytes"
if [ $NEEDED_WHITESPACE -gt 0 ]; then
  dd if=/dev/zero of=bin/boot1-new bs=1 seek=$SIZE_BOOT1 count=$NEEDED_WHITESPACE conv=notrunc
else
  echo NEEDED_WHITESPACE is $NEEDED_WHITESPACE, this is abnormal and might indicate a problem with your directory structure
fi

function padubi() {
    echo "Padding squashfs"
    SIZEUBI=$(wc -c bin/bootubi.new | awk '{print $1}')
    PAD=$((0x800000-$SIZEUBI))
    if [ $PAD -lt 0 ]; then
        echo "squashfs is too large :("
    else
        cp bin/bootubi.new bin/bootubi
        dd if=/dev/zero of=bin/bootubi seek=$SIZEUBI bs=1 count=$PAD conv=notrunc
    fi
}

padubi
cat bin/loader1 bin/boot1-new bin/bootubi bin/jffs2 > switch-new.bin

Size=$(wc -c switch-new.bin | awk '{print $1}')
if [ $Size -eq 16777216 ] ; then
    echo "switch-new.bin generated."
else
    echo "Error: switch-new.bin has an invalid size."
fi

