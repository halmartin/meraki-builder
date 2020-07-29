#!/usr/bin/env bash

DUMP_FILE=${1}

if [ "$DUMP_FILE" == "" ]; then
  DUMP_FILE=mtd12-original.dat
fi

test -d bin || mkdir bin

command -v find_hdr
if [ $? -ne 0 ]; then
  # use python
  RAMDISK_OFFSET=$(python3 ../tools/find_hdr.py -g $DUMP_FILE)
else
  # someone compiled my garbage C!
  RAMDISK_OFFSET=$(find_hdr -g $DUMP_FILE)
fi
# Extract mtd12 or mtd13's ramdisk
dd if=$DUMP_FILE of=bin/mtd12-header  bs=1024 count=1
dd if=$DUMP_FILE of=bin/mtd12-vmlinux bs=1 skip=1024 count=$((RAMDISK_OFFSET-1024))
dd if=$DUMP_FILE of=ramdisk.gz     bs=1 skip=$((RAMDISK_OFFSET))
# Yeah, I'm not convinced of Meraki's instructions that rootlist is empty, given the footer contents
#dd if=$DUMP_FILE of=bin/mtd12-post bs=1 count=$((0xf70)) skip=$((0x04af758 + 0xc626e2))

# Extract the ramdisk
test -d ramdisk && rm -rf ramdisk
if [ $(file ramdisk.gz | grep -c gzip) -eq 1 ]; then
    mkdir ramdisk
    cd ramdisk
    cat ../ramdisk.gz | gzip -d | cpio -i
else
    echo "ramdisk.gz is not a valid gzip archive!"
fi

