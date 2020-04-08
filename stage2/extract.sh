#!/usr/bin/env bash

DUMP_FILE=${1}

test -d bin || mkdir bin

RAMDISK_OFFSET=$(find_hdr -g mtd12-original.dat)
# Extract mtd12 or mtd13's ramdisk
dd if=mtd12-original.dat of=bin/mtd12-header  bs=1024 count=1
dd if=mtd12-original.dat of=bin/mtd12-vmlinux bs=1 skip=1024 count=$((RAMDISK_OFFSET-1024))
dd if=mtd12-original.dat of=ramdisk.gz     bs=1 skip=$((RAMDISK_OFFSET))
# Yeah, I'm not convinced of Meraki's instructions that rootlist is empty, given the footer contents
#dd if=mtd12-original.dat of=bin/mtd12-post bs=1 count=$((0xf70)) skip=$((0x04af758 + 0xc626e2))

# Extract the ramdisk
test -d ramdisk && rm -rf ramdisk
if [ $(file ramdisk.gz | grep -c gzip) -eq 1 ]; then
    mkdir ramdisk
    cd ramdisk
    cat ../ramdisk.gz | gzip -d | cpio -i
else
    echo "ramdisk.gz is not a valid gzip archive!"
fi

