#!/bin/sh

mkdir -p ${TARGET_DIR}/overlay
echo "Created /overlay"
mkdir -p ${TARGET_DIR}/click
echo "Created /click"

# remove dropbear, it will be created on JFFS2 overlay
if [ -h ${TARGET_DIR}/etc/dropbear ]; then
    rm ${TARGET_DIR}/etc/dropbear
    echo "Removed /etc/dropbear symlink"
fi

# set the lsb-release
IMAGE_DATE=$(date +%Y%m%d)
mkdir -p ${TARGET_DIR}/etc
cat << EOF > ${TARGET_DIR}/etc/lsb-release
DISTRIB_ID="postmerkOS"
DISTRIB_RELEASE="$IMAGE_DATE"
DISTRIB_DESCRIPTION="postmerkOS mipsel"
EOF

# get the salt from /etc/shadow and replace it in the initscript
salt=$(grep -E '^root' ${TARGET_DIR}/etc/shadow | awk -F'$' '{print $3}')
if [ -f ${TARGET_DIR}/etc/init.d/S14passwd ]; then
    sed -i "s/__SALT__/${salt}/g" ${TARGET_DIR}/etc/init.d/S14passwd
    echo "Changed salt to \"${salt}\" in /etc/init.d/S14passwd"
fi