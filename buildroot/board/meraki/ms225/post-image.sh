#!/bin/bash

BR2_IMAGES_DIR=$1

#echo IMAGES_DIR: $BR2_IMAGES_DIR
#echo BASE_DIR: $BASE_DIR
#echo BUILD_DIR: $BUILD_DIR
#echo TARGET_DIR: $TARGET_DIR
#echo STAGING_DIR: $STAGING_DIR
#echo BINARIES_DIR: $BINARIES_DIR
#echo HOST_DIR: $HOST_DIR

create_uboot() {
    U_BOOT_REGION=0xD0000
    U_BOOT_ENV_OFFSET=0xC0000
    U_BOOT_OUTPUT=${BINARIES_DIR}/u-boot.region
    if [ -f ${BINARIES_DIR}/u-boot-dtb.bin ] && [ -f ${BINARIES_DIR}/uboot-env.bin ]; then
        U_BOOT_SIZE=$(stat --format "%s" ${BINARIES_DIR}/u-boot-dtb.bin)
        U_BOOT_ENV_SIZE=$(stat --format "%s" ${BINARIES_DIR}/uboot-env.bin)
        dd if=/dev/zero of=$U_BOOT_OUTPUT bs=$(($U_BOOT_REGION)) count=1
        # copy u-boot-dtb.bin into the output region
        dd if=${BINARIES_DIR}/u-boot-dtb.bin of=$U_BOOT_OUTPUT bs=$(($U_BOOT_SIZE)) count=1 conv=notrunc
        # copy u-boot.env into the output region
        dd if=${BINARIES_DIR}/uboot-env.bin of=$U_BOOT_OUTPUT bs=$(($U_BOOT_ENV_SIZE)) seek=$(($U_BOOT_ENV_OFFSET/$U_BOOT_ENV_SIZE)) count=1 conv=notrunc 
        # copy u-boot.env into the backup region
        dd if=${BINARIES_DIR}/uboot-env.bin of=$U_BOOT_OUTPUT bs=$(($U_BOOT_ENV_SIZE)) seek=$(($(($U_BOOT_ENV_OFFSET/$U_BOOT_ENV_SIZE))+1)) count=1 conv=notrunc 
        # confirm that u-boot is the correct size
        U_BOOT_REGION_SIZE=$(stat --format "%s" $U_BOOT_OUTPUT)
        if [ $(($U_BOOT_REGION)) -ne $U_BOOT_REGION_SIZE ]; then
            echo "Generated u-boot region is ${U_BOOT_REGION_SIZE} but should be $(($U_BOOT_REGION))"
            return
        fi
        echo "u-boot.region: $(stat --format \"%s\" $U_BOOT_OUTPUT)"
    else
        echo "${BINARIES_DIR}/u-boot-dtb.bin or ${BINARIES_DIR}/uboot-env.bin not found"
        return
    fi
}

create_jffs2() {
    JFFS2_OUTPUT=${BINARIES_DIR}/jffs2.region
    JFFS2_SIZE=0x500000
    if [ $(grep -c BR2_PACKAGE_HOST_MTD=y ${BASE_DIR}/../.config) -ne 1 ]; then
        echo "Enable BR2_PACKAGE_HOST_MTD=y"
        return
    fi
    if [ ! -f ${HOST_DIR}/sbin/mkfs.jffs2 ]; then
        echo "${HOST_DIR}/sbin/mkfs.jffs2 does not exist"
        return
    fi
    TMPDIR=$(mktemp -d)
    mkdir -p $TMPDIR/jffs2-root/.upper/etc $TMPDIR/jffs2-root/.work/etc
    mkdir -p $TMPDIR/jffs2-root/.upper/root $TMPDIR/jffs2-root/.work/root
    echo "Generating JFFS2..."
    $HOST_DIR/sbin/mkfs.jffs2 --pad=${JFFS2_SIZE} -l -n -X lzo -x zlib -y 40:lzo -r $TMPDIR/jffs2-root/ -o ${JFFS2_OUTPUT}
    rm -r $TMPDIR
    echo "jffs2.region: $(stat --format \"%s\" $JFFS2_OUTPUT)"
}

create_kernel() {
    KERNEL_REGION=0x100000
    KERNEL_OUTPUT=${BINARIES_DIR}/kernel.region

    if [ ! -f ${BUILD_DIR}/linux-custom/vmlinuz.bin ]; then
        echo "${BUILD_DIR}/linux-custom/vmlinuz.bin does not exist"
        echo "Replacing kernel with zero image, you must netboot!"
	dd if=/dev/zero of=$KERNEL_OUTPUT bs=$(($KERNEL_REGION)) count=1
        return
    fi

    KERNEL_SIZE=$(stat --format "%s" ${BUILD_DIR}/linux-custom/vmlinuz.bin)
    dd if=/dev/zero of=/tmp/kernel.pad bs=$(($KERNEL_REGION-$KERNEL_SIZE)) count=1 > /dev/null 2>&1
    cat ${BUILD_DIR}/linux-custom/vmlinuz.bin /tmp/kernel.pad > $KERNEL_OUTPUT
    rm /tmp/kernel.pad
    echo "kernel.region: $(stat --format \"%s\" $KERNEL_OUTPUT)"
}

create_squashfs() {
    SQUASHFS_REGION=0x800000
    SQUASHFS_OUTPUT=${BINARIES_DIR}/squashfs.region
    if [ ! -f ${BINARIES_DIR}/rootfs.squashfs ]; then
        echo "${BINARIES_DIR}/rootfs.squashfs does not exist"
        return
    fi
    SQUASHFS_SIZE=$(stat --format "%s" ${BINARIES_DIR}/rootfs.squashfs)
    if [ $SQUASHFS_SIZE -gt $(($SQUASHFS_REGION)) ]; then
        echo "Squashfs exceeds maximum size ($SQUASHFS_REGION); cannot create image"
        return
    fi
    dd if=/dev/zero of=/tmp/squashfs.pad bs=$(($SQUASHFS_REGION-$SQUASHFS_SIZE)) count=1 > /dev/null 2>&1
    cat ${BINARIES_DIR}/rootfs.squashfs /tmp/squashfs.pad > ${SQUASHFS_OUTPUT}
    rm /tmp/squashfs.pad
    echo "squashfs.region: $(stat --format \"%s\" ${SQUASHFS_OUTPUT})"
}

create_uboot_flashable_image() {
    create_uboot
    create_kernel
    create_squashfs
    create_jffs2
    if [ ! -f ${BINARIES_DIR}/u-boot.region ] || [ ! -f ${BINARIES_DIR}/kernel.region ] || [ ! -f ${BINARIES_DIR}/squashfs.region ] || [ ! -f ${BINARIES_DIR}/jffs2.region ]; then
        echo "Image generation failed"
        return
    fi
    cat ${BINARIES_DIR}/u-boot.region ${BINARIES_DIR}/kernel.region ${BINARIES_DIR}/squashfs.region ${BINARIES_DIR}/jffs2.region > ${BINARIES_DIR}/switch-image.rom
    IMAGE_SIZE=$(stat --format "%s" ${BINARIES_DIR}/switch-image.rom)
    if [ ${IMAGE_SIZE} -ne 16777216 ]; then
        echo "Image generation failed"
    else
        echo "Flashable image: ${BINARIES_DIR}/switch-image.rom"
    fi
}

create_redboot_flashable_image() {
    download_redboot
    create_redboot_header
    create_squashfs
    create_jffs2
}

# create image with u-boot bootloader
create_uboot_flashable_image

# WIP:
# create image with redboot bootloader
# note that kernels compiled for RedBoot **must** have the boot commandline
# compiled into the kernel
#create_redboot_flashable_image
