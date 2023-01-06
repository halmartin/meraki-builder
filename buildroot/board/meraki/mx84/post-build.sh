#!/bin/sh

TARGET=$1

# allow login as root with password
if [ -f ${TARGET}/etc/ssh/sshd_config ]; then
    echo "PermitRootLogin yes" >> ${TARGET}/etc/ssh/sshd_config
fi
