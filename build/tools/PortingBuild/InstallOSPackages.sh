#!/bin/bash

OS=`cat /etc/issue.net | awk 'NR==1 {print $1}' | tr '[:upper:]' '[:lower:]'`
OSPATH=OS-dependent

echo "================================================"
echo "      Install Required Packages"
echo "================================================"
chmod 755 ${OSPATH}/${OS}.sh
${OSPATH}/${OS}.sh
if [ "$?" -ne "0" ]; then
  echo "Install Packages Error!!!!"
  exit 1
fi
