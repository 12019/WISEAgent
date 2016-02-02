#!/bin/bash
#----------------------------------------------------------
# Arg1: version.
#----------------------------------------------------------
HOME=${0%/*}
SRC_ROOT=${HOME}/../..
TOOLS_ROOT=${HOME}/../tools
INCLUDE_ROOT=${SRC_ROOT}/Include
STANDARD_ROOT=${HOME}/../Standard

version="0.0.0.0"
MAIN_VERSION=3
SUB_VERSIOM=0
BUILD_VERSION=0
REVISION=0

if [ $# -ne 0  ]; then
        version="$1"
	if [ -z $version ]; then
		source ${STANDARD_ROOT}/release_version
		REVISION=`"${TOOLS_ROOT}/GetSVNVersion.sh" ${SRC_ROOT}`
		version="$MAIN_VERSION.$SUB_VERSIOM.$BUILD_VERSION.$REVISION"
	else
		IFS='.' a=($version);
		MAIN_VERSION=${a[0]}
		SUB_VERSIOM=${a[1]}
		BUILD_VERSION=${a[2]}
		if [ -z "${a[3]}" ]; then
			REVISION=`"${TOOLS_ROOT}/GetSVNVersion.sh" ${SRC_ROOT}`
			version="$MAIN_VERSION.$SUB_VERSIOM.$BUILD_VERSION.$REVISION"
		else
			REVISION=${a[3]}
		fi
	fi
	#echo $version
else
	if [ -f ${STANDARD_ROOT}/release_version ]; then
		source ${STANDARD_ROOT}/release_version
		REVISION=`"${TOOLS_ROOT}/GetSVNVersion.sh" ${SRC_ROOT}`
		version="$MAIN_VERSION.$SUB_VERSIOM.$BUILD_VERSION.$REVISION"
	fi
        #echo $version
fi

if [ -z $REVISION ]; then
	REVISION=0
fi

#-------------------------------------
# SVN revision for C/C++
#-------------------------------------
INC_SVN_VER=${INCLUDE_ROOT}/svnversion.h
MAKE_SO_VER=${SRC_ROOT}/common_version.mk

echo "#ifndef __SVN_REVISION_H__" > "$INC_SVN_VER"
echo "#define __SVN_REVISION_H__" >> "$INC_SVN_VER"
echo "#define MAIN_VERSION $MAIN_VERSION" >> "$INC_SVN_VER"
echo "#define SUB_VERSION $SUB_VERSIOM" >> "$INC_SVN_VER"
echo "#define BUILD_VERSION $BUILD_VERSION" >> "$INC_SVN_VER"
echo "#define SVN_REVISION $REVISION" >> "$INC_SVN_VER"
echo "#endif /* __SVN_REVISION_H__ */" >> "$INC_SVN_VER"

echo "core_lib_SOVERSION := $version" > "$MAKE_SO_VER"
echo "SOVERSION := $version" >> "$MAKE_SO_VER"
echo "$version"

if [ -z "$2" ]; then
	AGENT_XML_VER=${SRC_ROOT}/build/Standard/config/agent_config.xml
else
	AGENT_XML_VER=$2
fi
AGENT_XML_TMP=${SRC_ROOT}/build/Standard/config/agent_config.xml.new
cp -af "${AGENT_XML_VER}" "${AGENT_XML_TMP}"
sed -i "s|\(<SWVersion>\)[^<>]*\(</SWVersion>\)|\1$version\2|" "${AGENT_XML_TMP}"
LISTEN_PORT=1883
if [[ $version == 3.0.* ]]; then
	LISTEN_PORT=10001
else
	LISTEN_PORT=1883
fi
sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$LISTEN_PORT\2|" "${AGENT_XML_TMP}"
SERVICE_NAME="saagent"
sed -i "s|\(<ServiceName>\)[^<>]*\(</ServiceName>\)|\1$SERVICE_NAME\2|" "${AGENT_XML_TMP}"


