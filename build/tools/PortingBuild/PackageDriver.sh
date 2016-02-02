#!/bin/bash

OS=`cat /etc/issue.net | awk 'NR==1 {print $1}' | tr '[:upper:]' '[:lower:]'`
ARCH=`uname -m | sed 's/x86_//;s/i[3-6]86/86/'`
VER=`cat ~/distribution.version`
DEST=$(basename `ls $HOME/*.tar.gz`)
FILENAME="${DEST##*/}"
EXTRACT=~/SUSI4.0*

echo "================================================"
echo "      Package SUSI Driver"
echo "================================================"
cd SourceCode/
make package
cp -f ~/SourceCode/*.tar.gz ~/.
if [ "$?" -ne "0" ]; then
  echo "Package Error!!!!"
  exit 1
fi

echo "================================================"
echo "      Extract Package"
echo "================================================"
tar -zxf *.tar.gz -C ~/.
if [ "$?" -ne "0" ]; then
  echo "Extract Error!!!!"
  exit 1
fi

echo "================================================"
echo "      Install Driver & Run SUSI4Demo tool"
echo "================================================"
cd ${EXTRACT}/Driver/
sudo make install
sudo ${EXTRACT}/Susi4Demo/susidemo4
if [ "$?" -ne "0" ]; then
  echo "Install Driver or Run Demo Tool Error!!!!"
  exit 1
fi

echo "================================================"
echo "      The List of Libs share objects"
echo "================================================"
ls ${EXTRACT}/Driver/lib* | awk -F "/" '{print $6}'

mv  ~/*.tar.gz ~/${PLATFORM}_${FILENAME%.*.*}_${OS}${VER}_x${ARCH}.tar.gz
echo "================================================"
echo "File: ${PLATFORM}_${FILENAME%.*.*}_${OS}${VER}_x${ARCH}.tar.gz"
echo "================================================"

