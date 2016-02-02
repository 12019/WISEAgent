#!/bin/bash

sudo yum install -y openssh subversion pciutils-devel libx86-devel redhat-lsb*
if [ "$?" -ne "0" ]; then
  echo "Install Packages Error!!!!"
  exit 1
fi

VER=`head -n1 /etc/fedora-release | awk '{print $3}'`
echo "${VER}" > ~/distribution.version
