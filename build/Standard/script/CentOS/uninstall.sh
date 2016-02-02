#!/bin/sh
echo "Uninstall AgentService." 

# user check; must be root
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "You must be root !"
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

if [ -f "/etc/init.d/sawebsockify" ]; then
  /etc/init.d/sawebsockify stop
  /sbin/chkconfig --del sawebsockify  
  rm /etc/init.d/sawebsockify
fi

if [ -f "/etc/init.d/sawatchdog" ]; then
  /etc/init.d/sawatchdog stop
  /sbin/chkconfig --del  sawatchdog  
  rm /etc/init.d/sawatchdog
fi

if [ -f "/etc/init.d/saagent" ]; then
  /etc/init.d/saagent stop
  /sbin/chkconfig --del  saagent  
  rm /etc/init.d/saagent
fi

if [ -d "$path" ]; then
  echo "remove folder $path"
  rm -rf "$path"  
  rm -f /var/run/$servername.pid
  rm -f /var/lock/subsys/cagent
  rm -f /var/run/SAWatchdog.pid
  rm -f /var/lock/subsys/sawatchdog
  rm -f /var/lock/subsys/sawebsockify
fi
wait
echo "done."
