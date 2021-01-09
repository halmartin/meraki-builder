## NAND firmware tools

Note: These tools are here if you wish to investigate the Meraki MS220-series switch firmware that resides on NAND, unless you need to extract kernel modules from a NAND dump or wish to investigate Meraki binaries, you can ignore this.

Contains scripts for extracting and modifying a dump of the UBI volume (called `part1` or `part2`; 21030912 bytes) which contains the running Meraki firmware.

If you are interested in examining the Meraki management binaries (e.g. `switch_brain` or `poe_server`) they are present in this firmware image.

This firmware contains pre-built kernel modules which (on my switch) are for the 3.18.123 kernel. You need to extract these modules as they are not included in the Meraki GPL archive.

If your switch is running a newer kernel than 3.18.123, demand the GPL archive corresponding to your kernel version from open-source@meraki.com !
