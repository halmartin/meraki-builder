This directory contains the kernel modules needed to initialize the Vitesse switch ASIC.

The following kernel modules are expected:
* elts_meraki.ko
* merakiclick.ko
* proclikefs.ko
* (luton26|jaguar|jaguar_dual)/vc_click.ko
* (luton26|jaguar|jaguar_dual)/vtss_core.ko

These modules can be extracted from your firmware dump (mtdblock12 or mtdblock13).

They MUST be for kernel 3.18.123-meraki-elemental, if you have a newer kernel version please email open-source@meraki.com and request the source code for your kernel version.
