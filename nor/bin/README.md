# bootloader

I suggest you use the patched version of RedBoot which doesn't check the CRC and allows you to boot a kernel larger than 3.9MB

https://github.com/halmartin/MS42-GPL-sources-3-18-122/blob/master/redboot/redboot-nocrc-sz.bin

It should be named `loader1`

# kernel

You can compile 3.18.123 yourself, the GPL source code is available: https://github.com/halmartin/switch-11-22-ms220/tree/master/linux-3.18

The kernel should be present in the `stage1` directory (the `..` to this directory).

The build script expects the stripped kernel (vmlinux.bin) but also requires the unstripped kernel to parse the entry address (vmlinux).

```
mipsel-linux-musl-objcopy -O binary -S vmlinux vmlinux.bin
```

# squashfs

Generate the squashfs by extracting the NAND firmware dump by using the scripts in the `stage2` directory.

Copy `bootubi.new` to this directory after running `stage2/make.sh`

# JFFS2

Reserved for future usage (e.g. storing the switch configuration). Currently not used, but required for building the firmware.

Just create it with dd:
```
dd if=/dev/zero of=jffs2 bs=3145728 count=1
```
