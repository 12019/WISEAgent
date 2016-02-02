#!/bin/bash

sudo apt-get update
sudo apt-get install -y ssh subversion build-essential libpci-dev libx86-dev
if [ "$?" -ne "0" ]; then
  echo "Install Packages Error!!!!"
  exit 1
fi

VER=`cat /etc/issue.net | awk '{print $2}'`
echo "${VER}" > ~/distribution.version
