#!/bin/sh
# Hal Martin (C) 2020
# https://github.com/halmartin/meraki-builder
# Used to modify a NAND dump from Meraki MS220 switches

RETC=ramdisk/etc
RUBIN=ramdisk/usr/bin
RSBIN=ramdisk/sbin
RUSBIN=ramdisk/usr/sbin
RULIB=ramdisk/usr/lib
RMODS=ramdisk/lib/modules
RROOT=ramdisk

function delete_kmods() {
    rm -rf $RMODS/jaguar
    rm -rf $RMODS/jaguar_dual
}

function delete_etc() {
    rm -f $RETC/hostapd*
}

function patch_etc() {
    sed -i 's;/usr/bin/serial_logincheck;/bin/ash;g' $RETC/passwd
    sed -i 's/\!//g' $RETC/passwd
    sed -i 's;/usr/bin/serial_logincheck --login;/bin/ash;g' $RETC/inittab
}

function delete_ubin() {
    rm -f $RUBIN/serial_logincheck
    rm -f $RUBIN/capture_kernel.sh
    rm -f $RUBIN/update_loader
    rm -f $RUBIN/test_*
    rm -f $RUBIN/test-*
    rm -f $RUBIN/mtunnel_*
    mv -f $RUBIN/config_updater $RUBIN/config_updater.bak
}

function delete_usbin() {
    rm -f $RUSBIN/bird
    rm -f $RUSBIN/birdc
    rm -f $RUSBIN/dhcpd
    rm -f $RUSBIN/hostapd*
    # don't need kexec since we're already booted
    rm -f $RUSBIN/kexec
    rm -f $RUSBIN/tcpdump
    rm -f $RUSBIN/wpa_passphrase
}

function delete_ulib() {
    rm -f $RULIB/meraki_snmp.so
}

function mount_config() {
    cat << EOF > $RETC/init.d/S20config
#!/bin/sh
mount -t jffs2 /dev/mtdblock4 /config
EOF
}

function create_var() {
    mkdir -p $RROOT/var/log
    mkdir -p $RROOT/var/run
}

delete_kmods
delete_etc
patch_etc
delete_ubin
delete_usbin
create_var

mksquashfs ./ramdisk/ bootubi.new -comp xz -all-root -noappend
