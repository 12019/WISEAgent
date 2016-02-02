#/bin/bash
source ../../../../risc-env fsl imx6 yocto
cd ../../
export TARGET_PLATFORM_NAME="Poky (Yocto Project Reference Distro) 1.5.3"
export TARGET_PLATFORM_ARCH="armv7l"
./autobuild.sh '' 'wise-3310'

#build upgrade package
./autobuild.sh '3.1.999' 'wise-3310'

