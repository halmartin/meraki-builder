#!/bin/sh

TARGET=$1

mkdir -p ${TARGET}/overlay
echo "Created /overlay"
mkdir -p ${TARGET}/click
echo "Created /click"

# remove dropbear, it will be created on JFFS2 overlay
if [ -h ${TARGET}/etc/dropbear ]; then
    rm ${TARGET}/etc/dropbear
    echo "Removed /etc/dropbear symlink"
fi
