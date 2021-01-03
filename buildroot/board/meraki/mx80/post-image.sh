#!/usr/bin/env bash

BR2_IMAGES_DIR=$1

# 0x4000: dtb
# 0x20000: uImage 
# 0x400000: initrd

create_part() {
    DTB_REGION=$((0x20000-0x4000))
    KERNEL_REGION=$((0x400000-0x20000))
    RD_REGION=$((0x18ffc00-0x400000))
    DTB_SIZE=$(stat --format "%s" ${BINARIES_DIR}/fullerene.dtb)
    dd if=/dev/zero of=${BINARIES_DIR}/dtb_pre.bin bs=$((0x4000-0x400)) count=1 >/dev/null 2>&1
    dd if=/dev/zero of=${BINARIES_DIR}/dtb_post.bin bs=$(($DTB_REGION-$DTB_SIZE)) count=1 >/dev/null 2>&1
    KERNEL_SIZE=$(stat --format "%s" ${BINARIES_DIR}/uImage)
    dd if=/dev/zero of=${BINARIES_DIR}/zero_uImage.bin bs=$(($KERNEL_REGION-$KERNEL_SIZE)) count=1 >/dev/null 2>&1
    RD_SIZE=$(stat --format "%s" ${BINARIES_DIR}/rootfs.cpio.uboot)
    cat ${BINARIES_DIR}/dtb_pre.bin ${BINARIES_DIR}/fullerene.dtb ${BINARIES_DIR}/dtb_post.bin ${BINARIES_DIR}/uImage ${BINARIES_DIR}/zero_uImage.bin ${BINARIES_DIR}/rootfs.cpio.uboot > ${BINARIES_DIR}/part1_data.bin
}

create_header() {
    #printf '\x8e\x73\xed\x8a\x00\x00\x04\x00\x01\x8f\xfc\x00' > ${BINARIES_DIR}/header1.bin
    printf '\x8e\x73\xed\x8a\x00\x00\x04\x00' > ${BINARIES_DIR}/header.bin
    SZD=$(stat --format "%s" ${BINARIES_DIR}/part1_data.bin | xargs printf '%08x')
    printf "\x$(echo $SZD | cut -b1-2)\x$(echo $SZD | cut -b3-4)\x$(echo $SZD | cut -b5-6)\x$(echo $SZD | cut -b7-8)" >> ${BINARIES_DIR}/header.bin
    SHA1=$(cat ${BINARIES_DIR}/part1_data.bin | sha1sum | awk '{print $1}')
    echo SHA1: $SHA1
    echo $SHA1 | xxd -r -p >> ${BINARIES_DIR}/header.bin
    printf '\xa1\xf0\xbe\xef\x00\x06\x00\x01\x00\x02\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x40\x00' >> ${BINARIES_DIR}/header.bin
    dd if=/dev/zero of=${BINARIES_DIR}/header_pad.bin bs=$((0x3c8)) count=1 >/dev/null 2>&1
}

create_image() {
    cat ${BINARIES_DIR}/header.bin ${BINARIES_DIR}/header_pad.bin ${BINARIES_DIR}/part1_data.bin > ${BINARIES_DIR}/ubi_image.bin
}

create_part
create_header
create_image
