## NOR firmware tools

This directory contains scripts for building the MS220 NOR firmware.

If you use [switch-11-22-ms220](https://github.com/halmartin/switch-11-22-ms220/) to build `vmlinuz`, you can create a flashable image with kernel modules (extracted below) to manage the switch ASIC.

Currently, the build script expects a SquashFS filesystem produced by buildroot. See the `buildroot` directory for the configuration and filesystem overlay.

The output of the `make.sh` script is a flashable image called `switch-new.bin`.
