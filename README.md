# meraki-builder
Scripts and tools to assemble firmware images for the following Meraki switches:
* MS220-8(P)
* MS220-24(P), MS220-48(LP|FP)
* MS22(P) and MS42(P)
* MS320-24(P) and MS320-48(LP|FP)

## Acknowledgements

The structure of this repository was inspired by the work of Leo Leung and his documentation:
https://leo.leung.xyz/wiki/Meraki_MS220-8P

However, the current target of this repository is a buildroot based firmware that does not reuse Meraki binaries for switch configuration/management.

Originally, "stage1" comes from a dump of the NOR firmware. As of late-2019, the NOR firmware is based on kernel 3.18.122 and does not contain the kernel modules necessary to configure the switching fabric.

## Build instructions

### RedBoot bootloader

Download the binary from [here](https://github.com/halmartin/MS42-GPL-sources-3-18-122/raw/master/redboot/redboot-nocrc-sz.bin) and save to `nor/bin/loader1`

### Kernel

Clone [switch-11-22-ms220](https://github.com/halmartin/switch-11-22-ms220) and follow the instructions in the README to build the kernel

### Rootfs

Download [buildroot](https://www.buildroot.org/download.html) and copy the contents of the `buildroot` directory in this repository over the buildroot source tree

After building the toolchain + target filesystem with `make`, copy `output/images/rootfs.squashfs` to `nor/bin/squashfs`

## NOR firmware tools

The `nor` directory contains scripts for building the NOR firmware.

If you use [switch-11-22-ms220](https://github.com/halmartin/switch-11-22-ms220/) to build `vmlinuz`, you can create a flashable image with kernel modules (extracted below) to manage the switch ASIC.

Currently, the build script expects a SquashFS filesystem produced by buildroot. See the `buildroot` directory for the configuration and filesystem overlay.

The output of the `make.sh` script is a flashable image called `switch-new.bin`.

## NAND firmware tools

Note: These tools are here if you wish to investigate the Meraki firmware that resides on NAND, unless you need to extract kernel modules from a NAND dump or wish to investigate Meraki binaries, you can ignore this.

Contains scripts for extracting and modifying a dump of the UBI volume (called `part1` or `part2`; 21030912 bytes) which contains the running Meraki firmware.

If you are interested in examining the Meraki management binaries (e.g. `switch_brain` or `poe_server`) they are present in this firmware image.

This firmware contains pre-built kernel modules which (on my switch) are for the 3.18.123 kernel. You need to extract these modules as they are not included in the Meraki GPL archive.

If your switch is running a newer kernel than 3.18.123, demand the GPL archive corresponding to your kernel version from open-source@meraki.com !
