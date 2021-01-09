# meraki-builder
Scripts and tools to assemble firmware images for the following Meraki switches:
* MS220-8(P)
* MS220-24(P), MS220-48(LP|FP)
* MS22(P) and MS42(P)
* MS320-24(P) and MS320-48(LP|FP)

There is also an overlay for building a firmware image for the Meraki MX80, see `buildroot/board/meraki/mx80`.

# Build instructions

## MS220-series switches

### RedBoot bootloader

Download the binary from [here](https://github.com/halmartin/MS42-GPL-sources-3-18-122/raw/master/redboot/redboot-nocrc-sz.bin) and save to `nor/bin/loader1`

### Kernel

Clone [switch-11-22-ms220](https://github.com/halmartin/switch-11-22-ms220) and follow the instructions in the README to build the kernel

### Rootfs

Download [buildroot](https://www.buildroot.org/download.html) and copy the contents of the `buildroot` directory in this repository over the buildroot source tree

After building the toolchain + target filesystem with `make`, copy `output/images/rootfs.squashfs` to `nor/bin/squashfs`

## MX80 router

### Docker

`cd docker/mx80 && make docker-build`

### Manually

* Download the latest stable release of [buildroot](https://www.buildroot.org/download.html)
* Copy `buildroot/board/meraki/mx80/buildroot-config` to `.config` in your extracted buildroot directory
* Copy `buildroot/board/merkai/mx80` to `board/meraki/mx80` in your buildroot tree
* Run `make` to build buildroot, the bootable/flashable image can be found in `output/images/ubi_image.bin`
