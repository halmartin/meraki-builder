# meraki-builder
Scripts and tools to assemble firmware images for Meraki MS220, MS22, MS42, MS320 switches

## Acknowledgements

The structure of this repository was inspired by the work of Leo Leung and his documentation:
https://leo.leung.xyz/wiki/Meraki_MS220-8P

However, the current target of this repository is a buildroot based firmware that does not reuse Meraki binaries for switch configuration/management.

Originally, "stage1" comes from a dump of the NOR firmware. As of late-2019, the NOR firmware is based on kernel 3.18.122 and does not contain the kernel modules necessary to configure the switching fabric.

## stage1

This directory contains scripts for building the NOR ("stage1") firmware. If you use switch-11-22-ms220 and this directory, you can create a flashable image with kernel modules to manage the switch ASIC.

The output of the `make.sh` script is a flashable image.

## stage2

Contains scripts for extracting and modifying a dump of the UBI volume (called `part1` or `part2`; 21030912 bytes) which contains the running Meraki firmware.

This firmware contains pre-built kernel modules which (on my switch) are for the 3.18.123 kernel. You need to extract these modules as they are not included in the Meraki GPL archive.

If your switch is running a newer kernel than 3.18.123, demand the GPL archive corresponding to your kernel version from open-source@meraki.com !

Currently, the stage1 build scripts expect a SquashFS filesystem produced by buildroot. See the `buildroot` directory for the configuration and filesystem overlay.

----

If you are interested in examining the Meraki management binaries (e.g. `switch_brain` or `poe_server`) they are present in this firmware image.

You will need to build find_hdr.c (used by `extract.sh`) or substitute `find_hdr.py` to extract the gzip compressed CPIO archive. 
