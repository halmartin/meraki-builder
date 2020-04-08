#!/usr/bin/env bash
# Script to decompile the _stock_ NOR firmware of Cisco Meraki MS220/320/22/42 switches

if [ $# -ne 1 ]; then
  echo "$0 <firmware_dump.bin>"
  exit 1
fi

test -d bin || mkdir bin

DUMP_FILE=${1}

# extract all sections of the NOR flash
dd if=$DUMP_FILE of=bin/loader1    bs=262144 skip=0  count=1
dd if=$DUMP_FILE of=bin/boot1      bs=262144 skip=1  count=15
dd if=$DUMP_FILE of=bin/loader2    bs=262144 skip=16 count=1
dd if=$DUMP_FILE of=bin/boot2      bs=262144 skip=17 count=15
dd if=$DUMP_FILE of=bin/rsvd       bs=262144 skip=32 count=2
dd if=$DUMP_FILE of=bin/bootubi    bs=262144 skip=34 count=24
dd if=$DUMP_FILE of=bin/conf       bs=262144 skip=58 count=1
dd if=$DUMP_FILE of=bin/stackconf  bs=262144 skip=59 count=4
dd if=$DUMP_FILE of=bin/syslog     bs=262144 skip=63 count=1

# Extract boot1 sections
# Spoiler: there are no boot1 sections.
# |--------|--------|--------|--------|
# |  SPIM  | KLOAD  |  LEN   | KENTRY |
# |--------|--------|--------|--------|
# | CRC32  |  ZERO  |  ZERO  |  ZERO  |
# |--------|--------|--------|--------|
# KERNEL + built-in INITRD:
# From CONFIG_BLK_DEV_INITRD + (rootlist)
# PADDING 0s up to 0x3c0000 (default size of boot1/boot2)
# the header is 32 bytes
dd if=bin/boot1 of=bin/boot1-header bs=$((0x20)) count=1
# extract the kernel
dd if=bin/boot1 of=bin/boot1-kernel       bs=1 skip=$((0x20))
# that's it! If you need to modify the contents of initrd, modify rootlist in the kernel src directory

echo "If you want to modify initrd, use rootlist in the 3.18.122 kernel source"
