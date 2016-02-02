#!/bin/sh
echo "Uninstall AgentService." 

path="/usr/local/AgentService"
SAAGENT_CONF="${path}/agent_config.xml"
servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $SAAGENT_CONF)
file="/var/run/$servername.pid"
if [ -f "$file" ]
then
	addpid=$(cat "$file")
	echo "Pid: $addpid"
	
	appdir=$(sudo readlink /proc/$addpid/cwd)
	if [ -d "$appdir" ]; then
	      echo "find app dir $appdir."
	      path=$appdir
	fi
else
	echo "$file not found."
fi
echo "AgentService Path: $path"

if [ -f "/etc/init.d/sawatchdog" ]; then
	sudo /etc/init.d/sawatchdog stop
	sudo /sbin/chkconfig --del  sawatchdog  
	sudo rm /etc/init.d/sawatchdog
fi

if [ -f "/etc/init.d/sawebsockify" ]; then
	sudo /etc/init.d/sawebsockify stop
	sudo /sbin/chkconfig --del  sawebsockify  
	sudo rm /etc/init.d/sawebsockify
fi

if [ -f "/etc/init.d/saagent" ]; then	
	sudo /etc/init.d/saagent stop 
	sudo /sbin/insserv -r /etc/init.d/saagent
	sudo systemctl --system daemon-reload
	sudo rm /etc/init.d/saagent
fi

if [ -d "$path" ]; then	
	echo "remove folder $path"
	rm -rf "$path"
  
	sudo rm /var/run/AgentService.pid
	sudo rm -f /var/run/SAWatchdog.pid
	sudo rm -f /var/run/sawebsockify.pid
fi
