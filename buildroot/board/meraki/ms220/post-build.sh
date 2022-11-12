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

# set the lsb-release
IMAGE_DATE=$(date +%Y%m%d)
mkdir -p ${TARGET}/etc
cat << EOF > ${TARGET}/etc/lsb-release
DISTRIB_ID="postmerkOS"
DISTRIB_RELEASE="$IMAGE_DATE"
DISTRIB_DESCRIPTION="postmerkOS mipsel"
EOF
