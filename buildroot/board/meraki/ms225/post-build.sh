#!/bin/sh

mkdir -p overlay
echo "Created /overlay"
mkdir -p click
echo "Created /click"

# remove dropbear, it will be created on JFFS2 overlay
if [ -h etc/dropbear ]; then
    rm etc/dropbear
    echo "Removed /etc/dropbear symlink"
fi
