#!/bin/bash

read -p "Input your platform name: " PLATFORM
read -p "Input your svn account: " USERNAME
read -p "Input your svn password: " PASSWD

./InstallOSPackages.sh
./CheckoutAndComplieSource.sh ${USERNAME} ${PASSWD}
./PackageDriver.sh
