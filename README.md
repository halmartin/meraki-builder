# meraki-builder
Scripts and tools to assemble firmware images for Meraki MS220, MS22, MS42, MS320 switches

## Acknowledgements

The scripts in this repository are heavily influenced by the work of Leo Leung and his documentation:
https://leo.leung.xyz/wiki/Meraki_MS220-8P

## stage1

Contains scripts for building the "stage1" firmware.

"stage1" comes from a dump of the NOR firmware. It does not contain the kernel modules necessary to configure the switching fabric.

The output of the `make.sh` script is a flashable image.

## stage2

Contains scripts for extracting and modifying the "stage2" firmware.

"stage2" comes from a dump of the NAND firmware.

This firmware contains pre-built kernel modules which (on my switch) are for the 3.18.123 kernel. You need to extract these modules as they are not included in the Meraki GPL archive.

If your switch is running a newer kernel than 3.18.123, demand the GPL archive corresponding to your kernel version from open-source@meraki.com !

The output of the `make.sh` script is a SquashFS filesytem. Use this with the "stage1" `make.sh` to create a flashable image.
