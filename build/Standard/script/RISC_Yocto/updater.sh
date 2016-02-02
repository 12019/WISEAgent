#!/bin/sh
echo "Install AgentService." 
CURDIR=${PWD} 
echo $CURDIR
TDIR=`mktemp -d`
cfgtmp=$TDIR/

#Add for Yocto
PATH_SYSTEMD="/usr/lib/systemd/system"  #or "/etc/systemd/system"
PATH_UPSTART="/etc/init"
PATH_SYSV="/etc/init.d"

SERVICE_NAME="saagent"
SERVICE_SYSTEMD="$SERVICE_NAME.service"
SERVICE_UPSTART="$SERVICE_NAME.conf"
SERVICE_SYSV="$SERVICE_NAME"

WATCHDOG_NAME="sawatchdog"
WATCHDOG_SYSTEMD="$WATCHDOG_NAME.service"
WATCHDOG_UPSTART="$WATCHDOG_NAME.conf"
WATCHDOG_SYSV="$WATCHDOG_NAME"

ACCOUNT_ROOT="`who | grep "root" | cut -d ' ' -f 1 | head -n 1`"

if [ $ACCOUNT_ROOT == "root" ] || [ -z "`command -v sudo`" ]; then 
	alias sudo=''
fi

function check_yocto()
{
	local retval=""
	V_YOCTO=`cat /etc/issue |grep Yocto`
	V_WRL=`cat /etc/issue |grep "Wind River Linux"`
	
	if [ -n "$V_YOCTO" ] || [ -n "$V_WRL" ];then # Yocto
		retval="yocto"
	else
		retval="x86"
	fi
	eval "$1=$retval"
}

xhost +

function clearcfgtmp()
{
	if [ -f "$cfgtmp" ]
	then
		echo "remove tmp dir $cfgtmp."
		sudo rm -rf "$cfgtmp"
	fi
}

function cfgbackup()
{
	echo "backup config files"
	
	clearcfgtmp
	cp -vf $(find $path  -maxdepth 1 -name '*.xml') "$cfgtmp"
}

function cfgrestore()
{
	if [ -d "$cfgtmp" ]
	then
		echo "restore config files"
		cp -vf $(find $cfgtmp -name '*.xml') "./AgentService/"
		rm -rf "$cfgtmp"
	fi
}

#Main()
check_yocto check_ver
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

	if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			echo "exec systemd init"
		elif [ -d "$PATH_UPSTART" ]; then
			echo "exec upstart init"
		else
			$PATH_SYSV/$SERVICE_SYSV stop
		fi	
	else
		$PATH_SYSV/$SERVICE_SYSV stop
	fi

	if [ -f ${SAAGENT_CONF} ]; then
		curversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $SAAGENT_CONF)
	fi

	cfgbackup;
else
	echo "$file not found."
fi

if [ -d "$path" ]; then
  echo "remove folder $path"
  rm -rf "$path"
fi

# Collect new config file information
INSTALL_AGENT_CONFIG="${CURDIR}/AgentService/agent_config.xml"
if [ -e ${INSTALL_AGENT_CONFIG} ]; then
	servicename=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $INSTALL_AGENT_CONFIG)
	newversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $INSTALL_AGENT_CONFIG)
	serverport=$(sed -n 's:.*<ServerPort>\(.*\)</ServerPort>.*:\1:p' $INSTALL_AGENT_CONFIG)
else
	echo "not found: ${INSTALL_AGENT_CONFIG}"
fi

echo "Copy AgentService to /usr/local."
cfgrestore;
if [ ! -d /usr/local ]; then
	mkdir -p /usr/local/
fi
cp -avr ./AgentService /usr/local

# Overwrite Server Port and Service Name
if [[ $curversion == 3.0.* ]]; then
	if [[ $newversion == 3.1.* ]]; then
		echo "Overwrite Server Port"
		sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$serverport\2|" $SAAGENT_CONF
		sed -i "s|\(<ServiceName>\)[^<>]*\(</ServiceName>\)|\1$servicename\2|" $SAAGENT_CONF
	fi
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "exec systemd init"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "exec upstart init"
	else
		cp ./$SERVICE_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$SERVICE_SYSV
		chmod 0750 $PATH_SYSV/$SERVICE_SYSV
	fi
else
	cp ./$SERVICE_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$SERVICE_SYSV
	chmod 0750 $PATH_SYSV/$SERVICE_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "move $SERVICE_SYSTEMD to $PATH_SYSTEMD"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "move $SERVICE_UPSTART to $PATH_UPSTART"
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $SERVICE_SYSV
			/sbin/insserv -v $PATH_SYSV/$SERVICE_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $SERVICE_SYSV remove
			update-rc.d $SERVICE_SYSV defaults 90 20
		elif [ -n "`command -v chkconfig`" ]; then
			chkconfig --del $SERVICE_SYSV
			chkconfig --add $SERVICE_SYSV
		else 
			echo "Please install insserv/update-rc.d/chkconfig tool."
		fi
	fi		
else
	sudo /sbin/insserv -v $PATH_SYSV/$SERVICE_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "exec systemd init"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "exec upstart init"
	else
		cp ./$WATCHDOG_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$WATCHDOG_SYSV
		chmod 0750 $PATH_SYSV/$WATCHDOG_SYSV
	fi
else
	sudo cp ./$WATCHDOG_SYSV $PATH_SYSV
	sudo chown root:root $PATH_SYSV/$WATCHDOG_SYSV
	sudo chmod 0750 $PATH_SYSV/$WATCHDOG_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "move $WATCHDOG_SYSTEMD to $PATH_SYSTEMD"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "move $WATCHDOG_UPSTART to $PATH_UPSTART"
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $WATCHDOG_SYSV
			/sbin/insserv -v $PATH_SYSV/$WATCHDOG_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $WATCHDOG_SYSV remove
			update-rc.d $WATCHDOG_SYSV defaults 90 20
		elif [ -n "`command -v chkconfig`" ]; then
			chkconfig --del $WATCHDOG_SYSV
			chkconfig --add $WATCHDOG_SYSV
		else 
			echo "Please install insserv/update-rc.d/chkconfig tool."
		fi
	fi		
else
	sudo /sbin/insserv -v $PATH_SYSV/$WATCHDOG_SYSV
fi

if [ "$check_ver" != "yocto" ]; then
	sudo systemctl --system daemon-reload
fi

if [ "$check_ver" != "yocto" ]; then
	find /home/ -name 'Autostart' -print0 | while IFS= read -r -d $'\0' line; do
		echo "$line"
		cp ./AgentService/xhostshare.sh "$line"
		chown --reference="$line" "$line/xhostshare.sh"
		chmod 755 "$line/xhostshare.sh"
	done
fi

read -t 10 -p "Setup Agent [y/n](default:n): " ans

if [ "$ans" == "y" ]; then
	sudo /usr/local/AgentService/setup.sh
else
	if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			echo "systemctl start $SERVICE_NAME"
		elif [ -d "$PATH_UPSTART" ]; then
			echo "initctl start $SERVICE_NAME"
		else
			echo "Start RMM Agent."
			$PATH_SYSV/$SERVICE_SYSV start

			echo "Start Watchdog Agent."
			$PATH_SYSV/$WATCHDOG_SYSV start
		fi
	else
		echo "Start RMM Agent."
		echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
		sudo $PATH_SYSV/$SERVICE_SYSV start
		sudo $PATH_SYSV/$WATCHDOG_SYSV start
	fi	
fi
