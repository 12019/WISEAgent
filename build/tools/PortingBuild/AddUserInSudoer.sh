#!/bin/bash

# This script must be executed under ROOT but not "a user with ROOT privilege(super user)".
# You can switch to ROOT by using these commands below.
# Fedora: su -
# Ubuntu: sudo -i

USER=`w | awk 'END {print $1}'`
HOME=/home/${USER}
IP=`ifconfig | grep '172.' | awk '{print $2}'`

usermod ${USER} -a -G wheel
echo ${USER} 'ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers

echo Add user ${USER} in Sudoer without passwd
echo "your IP: "${IP}

chmod 755 ${HOME}/*.sh
chmod 755 ${HOME}/OS-dependent/*.sh
