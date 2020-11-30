#!/bin/sh

# remove dropbear, it will be created on JFFS2 overlay
if [ -h etc/dropbear ]; then
    rm etc/dropbear
fi
