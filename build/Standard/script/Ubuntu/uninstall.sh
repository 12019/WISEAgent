#!/bin/bash
echo "Uninstall AgentService." 

# user check; must be root
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "Permission denied!"
	exit -1
fi

path="/usr/local/AgentService"
SAAGENT_CONF="${path}/agent_config.xml"
servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $SAAGENT_CONF)

file="/var/run/$servername.pid"
if [ -f "$file" ]
then
	addpid=$(cat "$file")
	echo "Pid: $addpid"
	
	appdir=$(readlink /proc/$addpid/cwd)
	if [ -d "$appdir" ]; then
	      echo "find app dir $appdir."
	      path=$appdir
	fi
else
	echo "$file not found."
fi
echo "AgentService Path: $path"

if [ -f "/etc/init/sawatchdog.conf" ]; then
	stop sawatchdog 
	rm -f "/etc/init/sawatchdog.conf"
fi

if [ -f "/etc/init/sawebsockify.conf" ]; then
	stop sawebsockify
	rm -f "/etc/init/sawebsockify.conf"
fi

if [ -f "/etc/init/saagent.conf" ]; then	
	stop saagent
	rm -f "/etc/init/saagent.conf"
fi

if [ -d "$path" ]; then
	echo "remove folder $path"
	rm -rf "$path"
	
	rm -f "/var/lock/subsys/sawatchdog"
	rm -f "/var/run/SAWatchdog.pid"
	rm -f "/var/lock/subsys/sawebsockify"
	rm -f "/var/run/sawebsockify.pid"
	rm -f "/var/lock/subsys/saagent"
	rm -f "/var/run/$servername.pid"
	
	rm -f "/etc/profile.d/xhostshare.sh"
fi
echo "done."
