#!/bin/sh
APPDIR=${PWD} 
echo $APPDIR

APP_CAGENT="${APPDIR}/cagent"

if [ -f ${APP_CAGENT} ]; then
	sadmin solidify "${APPDIR}"
	sadmin updaters add "${APP_CAGENT}"
else
	echo "not found: ${APP_CAGENT}"
fi
