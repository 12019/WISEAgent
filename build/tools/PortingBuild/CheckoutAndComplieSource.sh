#!/bin/bash

USERNAME=$1
PASSWD=$2

echo "================================================"
echo "      Check Out SUSI Driver from SVN"
echo "================================================"
svn co https://172.20.2.44/svn/ess/SUSI/SUSI_4.0//SUSI/SourceCode \
--username ${USERNAME} --password ${PASSWD}
if [ "$?" -ne "0" ]; then
  echo "Check out SUSI Driver Error!!!!"
  exit 1
fi

echo "================================================"
echo "      Build SUSI Driver SourceCode"
echo "================================================"
cd SourceCode/
make
if [ "$?" -ne "0" ]; then
  echo "Build SUSI Driver Error!!!!"
  exit 1
fi
