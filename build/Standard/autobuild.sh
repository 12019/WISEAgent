#!/bin/bash
HOME=${PWD}
SRC_ROOT=${HOME}/../..
TOOLS_ROOT=${HOME}/../tools
MAKESELF_TOOL_PATH=${TOOLS_ROOT}/makeself-2.1.5
CONFIG_PATH=${HOME}/config
DOC_PATH=${HOME}/doc
MODULE_CFG_PATH=${CONFIG_PATH}/module/linux
SCRIPT_PATH=${HOME}/script
OUTPUT_PATH=${SRC_ROOT}/Release
CAGENT_OUTPUT_PATH=${OUTPUT_PATH}/AgentService
UPDATER_SHELL_FILE=updater.sh
PRE_INSTALL_CHK_FILE=pre-install_chk.sh
UNINSTALL_SHELL_FILE=uninstall.sh
SETUP_SHELL_FILE=setup.sh
NETINFO_SHELL_FILE=netInfo.sh
PACKED_PATH=${SRC_ROOT}/Wrapped
PACKAGE_FOLDER_NAME=WISEAgent
SRC_SOURCE_ROOT=${SRC_ROOT}/build/Standard/WISEAgent
SRC_SOURCE_INCLUDE=${SRC_ROOT}/Include
SRC_SOURCE_PLATFORM=${SRC_ROOT}/Platform
SRC_SOURCE_SAMPLE=${SRC_ROOT}/Sample
SRC_SOURCE_LIBRARY=${SRC_ROOT}/Library
SRC_SOURCE_THIRDLIBRARY=${SRC_ROOT}/Library3rdParty
TARGET_SOURCE_ROOT=${SRC_ROOT}/Output/${PACKAGE_FOLDER_NAME}
TARGET_SOURCE_DOC=${TARGET_SOURCE_ROOT}/Doc
TARGET_SOURCE_INCLUDE=${TARGET_SOURCE_ROOT}/Include
TARGET_SOURCE_PLATFORM=${TARGET_SOURCE_ROOT}/Platform
TARGET_SOURCE_SAMPLE=${TARGET_SOURCE_ROOT}/Sample
TARGET_SOURCE_LIBRARY=${TARGET_SOURCE_ROOT}/Library
TARGET_SOURCE_THIRDLIBRARY=${TARGET_SOURCE_ROOT}/Library3rdParty
version="0.0.0.0"

function pause(){
        read -n 1 -p "$*" INP
        if [ -n $INP ] ; then
                echo -ne '\b \n'
        fi
}

#-------------------------------------
# Change file mode to 755
#-------------------------------------
cd ${HOME}/../
${HOME}/../recv_chmod.sh

cd ${HOME}
#pause "recv chmod"

#-------------------------------------
# Target Device Info
#-------------------------------------
TARGET_DEVICE=$2
echo "Target Device: $TARGET_DEVICE"
#pause "Target Device: $TARGET_DEVICE"

#-------------------------------------
# Generate Version Info File
#-------------------------------------
version=`"${TOOLS_ROOT}/GenVersionInfo.sh" "$1" "${CONFIG_PATH}/agent_config${TARGET_DEVICE}.xml"`
echo "Version: $version"
#pause "Version: $version"

#-------------------------------------
# Get Platform Info
#-------------------------------------
if [ "$RISC_TARGET" == "risc" ] && [ "$OS_VERSION" == "yocto" ]; then
	platform="$RISC_TARGET"_"$OS_VERSION"
	ARCH="$CHIP_VANDER"_"$PLATFORM"
	VER="$OS_VERSION"
else
	platform=$(lsb_release -ds | sed 's/\"//g')
	ARCH=$(uname -m)
	#ARCH=$(uname -m | sed 's/x86_//;s/i[3-6]86/32/')
	VER=$(lsb_release -sr)
fi

SCRIPT_TARGET_PLATFORM="windows"
case $platform in
  openSUSE* )
    SCRIPT_TARGET_PLATFORM="openSUSE"
    ;;
  CentOS* )
    SCRIPT_TARGET_PLATFORM="CentOS"
    ;;  
  Ubuntu* )
    SCRIPT_TARGET_PLATFORM="Ubuntu"
    ;;
  risc_yoct* )
    SCRIPT_TARGET_PLATFORM="RISC_Yocto"
    ;;
esac

echo "Platform: $platform"
echo "Architecture: $ARCH"
echo "OS Version: $VER"

#sudo ${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${PRE_INSTALL_CHK_FILE} -devel

#pause "pre-check"

if [ -d "${OUTPUT_PATH}" ]; then
  rm -rf "${OUTPUT_PATH}"
fi

make -C "${SRC_ROOT}" clean

#-------------------------------------
# RMM Agent Library
#-------------------------------------
if [ -d "${SRC_ROOT}/Output" ]; then
  rm -rf "${SRC_ROOT}/Output"
fi
mkdir "${SRC_ROOT}/Output"

#mkdir "${TARGET_SOURCE_ROOT}"
cp -af "${SRC_SOURCE_ROOT}" "${TARGET_SOURCE_ROOT}"
cp -af "${SRC_ROOT}/common_version.mk" "${TARGET_SOURCE_ROOT}/"

mkdir "${TARGET_SOURCE_DOC}"
#cp -af ${DOC_PATH}/${SCRIPT_TARGET_PLATFORM}/*		"${TARGET_SOURCE_DOC}"
#cp -af `find ${DOC_PATH}/ -maxdepth 1 -type f`		"${TARGET_SOURCE_DOC}"
find ${DOC_PATH}/ -maxdepth 1 -type f -print0 | xargs -0 -n 1 -Ifoo cp -af foo "${TARGET_SOURCE_DOC}"


mkdir "${TARGET_SOURCE_SAMPLE}"
cp -af "${SRC_SOURCE_SAMPLE}" "${TARGET_SOURCE_ROOT}/"

mkdir "${TARGET_SOURCE_INCLUDE}"
cp -af "${SRC_SOURCE_INCLUDE}" "${TARGET_SOURCE_ROOT}/"

mkdir "${TARGET_SOURCE_PLATFORM}"
cp -af "${SRC_SOURCE_PLATFORM}" "${TARGET_SOURCE_ROOT}/"

if [ ! -d "${TARGET_SOURCE_LIBRARY}" ]; then
  mkdir "${TARGET_SOURCE_LIBRARY}"
fi
mkdir "${TARGET_SOURCE_LIBRARY}/cJSON"
cp -af "${SRC_SOURCE_LIBRARY}/cJSON" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/FtpHelper"
cp -af "${SRC_SOURCE_LIBRARY}/FtpHelper" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/Log"
cp -af "${SRC_SOURCE_LIBRARY}/Log" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/MD5"
cp -af "${SRC_SOURCE_LIBRARY}/MD5" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/DES"
cp -af "${SRC_SOURCE_LIBRARY}/DES" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/Base64"
cp -af "${SRC_SOURCE_LIBRARY}/Base64" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/MessageGenerator"
cp -af "${SRC_SOURCE_LIBRARY}/MessageGenerator" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/SAClient"
cp -af "${SRC_SOURCE_LIBRARY}/SAClient" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/SAConfig"
cp -af "${SRC_SOURCE_LIBRARY}/SAConfig" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/SAGeneralHandler"
cp -af "${SRC_SOURCE_LIBRARY}/SAGeneralHandler" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/SAHandlerLoader"
cp -af "${SRC_SOURCE_LIBRARY}/SAHandlerLoader" "${TARGET_SOURCE_LIBRARY}/"
	
mkdir "${TARGET_SOURCE_LIBRARY}/SAManager"
cp -af "${SRC_SOURCE_LIBRARY}/SAManager" "${TARGET_SOURCE_LIBRARY}/"

mkdir "${TARGET_SOURCE_LIBRARY}/MQTTHelper"
cp -af "${SRC_SOURCE_LIBRARY}/MQTTHelper" "${TARGET_SOURCE_LIBRARY}/"

if [ ! -d "${TARGET_SOURCE_THIRDLIBRARY}" ]; then
  mkdir "${TARGET_SOURCE_THIRDLIBRARY}"
fi
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/curl-7.37.1"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/curl-7.37.1" "${TARGET_SOURCE_THIRDLIBRARY}/"
	
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/libxml2-2.7.8"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/libxml2-2.7.8" "${TARGET_SOURCE_THIRDLIBRARY}/"
	
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/libxml2-2.7.8.win32"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/libxml2-2.7.8.win32" "${TARGET_SOURCE_THIRDLIBRARY}/"
	
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/mosquitto-1.3.4"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/mosquitto-1.3.4" "${TARGET_SOURCE_THIRDLIBRARY}/"
	
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/openssl-1.0.1h"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/openssl-1.0.1h" "${TARGET_SOURCE_THIRDLIBRARY}/"
	
mkdir "${TARGET_SOURCE_THIRDLIBRARY}/openssl-1.0.1h.win32"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/openssl-1.0.1h.win32" "${TARGET_SOURCE_THIRDLIBRARY}/"

mkdir "${TARGET_SOURCE_THIRDLIBRARY}/pthreads.win32"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/pthreads.win32" "${TARGET_SOURCE_THIRDLIBRARY}/"

mkdir "${TARGET_SOURCE_THIRDLIBRARY}/log4z-3.2.0"
cp -af "${SRC_SOURCE_THIRDLIBRARY}/log4z-3.2.0" "${TARGET_SOURCE_THIRDLIBRARY}/"

#-------------------------------------
# Configure Environment
#-------------------------------------
LIB_SMART_TOOLS_DIR=${SRC_SOURCE_THIRDLIBRARY}/Smart/smartmontools
LIB_JPEG_DIR=${SRC_SOURCE_THIRDLIBRARY}/jpeg-8d1
LIB_VNC_DIR=${SRC_SOURCE_THIRDLIBRARY}/x11vnc-0.9.3
LIB_WEBSOCKIFY_DIR=${SRC_SOURCE_THIRDLIBRARY}/Websockify
VNC_EXEC_FILE=${LIB_VNC_DIR}/x11vnc/x11vnc
WEBSOCKIFY_EXEC_FILE=${LIB_WEBSOCKIFY_DIR}/websockify


if [ "${RISC_TARGET}" == "risc" ] ; then
  if [ "$CHIP_VANDER" == "fsl" ] && [ "$PLATFORM" == "imx6" ]; then
       HOST="arm-poky-linux-gnueabi"
  elif [ "$CHIP_VANDER" == "intel" ] && [ "$PLATFORM" == "quark" ]; then
  		 HOST="i586-poky-linux"
  elif [ "$CHIP_VANDER" == "intel" ] && [ "$PLATFORM" == "idp" ]; then
       HOST="x86_64-wrs-linux"
  fi
for d in ${LIB_SMART_TOOLS_DIR} ${LIB_VNC_DIR}; do (cd "$d" && chmod 755 configure && ./configure --host $HOST); done
for d in ${LIB_JPEG_DIR}; do (cd "$d" && chmod 755 configure && ./configure --host $HOST --disable-shared); done
else
for d in ${LIB_SMART_TOOLS_DIR} ${LIB_VNC_DIR}; do (cd "$d" && chmod 755 configure && ./configure); done
for d in ${LIB_JPEG_DIR}; do (cd "$d" && chmod 755 configure && ./configure --disable-shared); done
fi
make -C "${LIB_JPEG_DIR}" clean
make -C "${LIB_JPEG_DIR}" 
cd ${HOME}

#-------------------------------------
# Build Code
#-------------------------------------
make -C "${SRC_ROOT}"
if [ $? -eq 0 ] ; then
	echo "make succeeds"
else
	echo "make fails"
	exit
fi
make -C "${SRC_ROOT}" install
if [ $? -eq 0 ] ; then
	echo "make install succeeds"
else
	echo "make install fails"
	exit
fi
#-------------------------------------
# Config and shell script
#-------------------------------------
if [ ! -d "${CAGENT_OUTPUT_PATH}" ]; then
  mkdir "${CAGENT_OUTPUT_PATH}"
fi

mv -f "${CONFIG_PATH}/agent_config.xml.new" "${CAGENT_OUTPUT_PATH}/agent_config.xml"
cp -af "${CAGENT_OUTPUT_PATH}/agent_config.xml" "${CAGENT_OUTPUT_PATH}/agent_config_def.xml"
if [ ! -d "${CAGENT_OUTPUT_PATH}/module" ]; then
  mkdir "${CAGENT_OUTPUT_PATH}/module"
fi

cp -af "${CONFIG_PATH}/logger.ini" "${CAGENT_OUTPUT_PATH}/logger.ini"
sed -i "s|path=./log/|path=/usr/local/AgentService/log/|g" "${CAGENT_OUTPUT_PATH}/logger.ini"

if [[ $version == 3.0.* ]]; then
	cp -af "${MODULE_CFG_PATH}/module_config_30.xml" "${CAGENT_OUTPUT_PATH}/module/module_config.xml"
else
	cp -af "${MODULE_CFG_PATH}/module_config${TARGET_DEVICE}.xml" "${CAGENT_OUTPUT_PATH}/module/module_config.xml"
fi
echo "ProcName=cagent;CommID=1;StartupCmdLine=saagent"          | tee    "${CAGENT_OUTPUT_PATH}/SAWatchdog_Config"
echo "ProcName=websockify;CommID=2;StartupCmdLine=sawebsockify" | tee -a "${CAGENT_OUTPUT_PATH}/SAWatchdog_Config"

cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${UPDATER_SHELL_FILE}"		"${OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/saagent"			"${OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/sawebsockify"			"${OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/sawatchdog"			"${OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${UNINSTALL_SHELL_FILE}"	"${CAGENT_OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${SETUP_SHELL_FILE}"		"${CAGENT_OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${NETINFO_SHELL_FILE}"		"${CAGENT_OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/xhostshare.sh"			"${CAGENT_OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/servicectl.sh"			"${CAGENT_OUTPUT_PATH}/"
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/McAfeeAddUpdater.sh"		"${CAGENT_OUTPUT_PATH}/"
[ -f "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${PRE_INSTALL_CHK_FILE}" ] && \
cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${PRE_INSTALL_CHK_FILE}"	"${CAGENT_OUTPUT_PATH}/"

mkdir -p "${CAGENT_OUTPUT_PATH}/doc"
cp -af ${DOC_PATH}/${SCRIPT_TARGET_PLATFORM}/*		"${CAGENT_OUTPUT_PATH}/doc"
#cp -af `find ${DOC_PATH}/ -maxdepth 1 -type f`		"${CAGENT_OUTPUT_PATH}/doc"
find ${DOC_PATH}/ -maxdepth 1 -type f -print0 | xargs -0 -n 1 -Ifoo cp -af foo "${CAGENT_OUTPUT_PATH}/doc"

#-------------------------------------
# Build 3rd party tools
#-------------------------------------
make -C "${LIB_SMART_TOOLS_DIR}" clean
make -C "${LIB_SMART_TOOLS_DIR}"
if [ $? -eq 0 ] ; then
	echo "make succeeds"
	cp -avf "${LIB_SMART_TOOLS_DIR}/smartctl" "${CAGENT_OUTPUT_PATH}/"
else
	echo "make fails"
	if [ "${RISC_TARGET}" != "risc" ] ; then
		exit
	fi
fi

make -C "${LIB_VNC_DIR}" clean
make -C "${LIB_VNC_DIR}"
if [ $? -eq 0 ] ; then
	echo "make succeeds"
	if [ ! -d "${CAGENT_OUTPUT_PATH}/VNC" ]; then
		mkdir "${CAGENT_OUTPUT_PATH}/VNC"
	fi

	cp -avf "${VNC_EXEC_FILE}" "${CAGENT_OUTPUT_PATH}/VNC"
else
	echo "make fails"
	if [ "${RISC_TARGET}" != "risc" ] ; then
		exit
	fi
fi
	
make -C "${LIB_WEBSOCKIFY_DIR}" clean
make -C "${LIB_WEBSOCKIFY_DIR}" 
if [ $? -eq 0 ] ; then
	echo "make succeeds"
	if [ ! -d "${CAGENT_OUTPUT_PATH}/VNC" ]; then
		mkdir "${CAGENT_OUTPUT_PATH}/VNC"
	fi

	cp -avf "${WEBSOCKIFY_EXEC_FILE}" "${CAGENT_OUTPUT_PATH}/VNC"
else
	echo "make fails"
	if [ "${RISC_TARGET}" != "risc" ] ; then
		exit
	fi
fi

#-------------------------------------
# Create SFX
#-------------------------------------

# Rename package name to match target platform
if [ ! -z "${TARGET_PLATFORM_NAME}" -a "${TARGET_PLATFORM_NAME}" != " " ]; then
        platform=${TARGET_PLATFORM_NAME}
fi

if [ ! -z "${TARGET_PLATFORM_ARCH}" -a "${TARGET_PLATFORM_ARCH}" != " " ]; then
        ARCH=${TARGET_PLATFORM_ARCH}
fi

PACKED_FILE_NAME="rmmagent-$platform $ARCH-$version.run"
PACKED_LIB_NAME="WISEAgent-$version"
echo "File Name: $PACKED_FILE_NAME"

"${MAKESELF_TOOL_PATH}/makeself.sh" "${OUTPUT_PATH}/" "${PACKED_FILE_NAME}" "The Installer for RMM Agent" "./${UPDATER_SHELL_FILE}"

if [ ! -d "${PACKED_PATH}" ]; then
  mkdir "${PACKED_PATH}"
fi
mv "${PACKED_FILE_NAME}" "${PACKED_PATH}/"
cp -af "${DOC_PATH}/${SCRIPT_TARGET_PLATFORM}/Install manual" "${PACKED_PATH}/"
tar -zcvf "${PACKED_PATH}/${PACKED_FILE_NAME}.tar.gz" "${PACKED_PATH}/Install manual" "${PACKED_PATH}/${PACKED_FILE_NAME}"
#tar -zcvf "${PACKED_PATH}/${PACKED_LIB_NAME}.tar.gz" "${TARGET_SOURCE_ROOT}"
cd "${SRC_ROOT}/Output"
find . -name ".svn" -exec rm -rf {} \;
zip -rD "${PACKED_LIB_NAME}.zip" "${PACKAGE_FOLDER_NAME}"
mv "${PACKED_LIB_NAME}.zip" "${PACKED_PATH}/"
rm -f "${PACKED_PATH}/Install manual"

