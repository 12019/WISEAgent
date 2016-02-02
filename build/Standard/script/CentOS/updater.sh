#!/bin/sh
echo "Install AgentService." 
CURDIR=${PWD} 
echo $CURDIR
TDIR=`mktemp -d`
cfgtmp=$TDIR/

xhost +

function clearcfgtmp()
{
	if [ -d "$cfgtmp" ]
	then
		echo "remove tmp dir $cfgtmp."
		rm -rf "$cfgtmp*"
	fi
}

function cfgbackup()
{
	echo "backup config files"
	
	clearcfgtmp
	cp -vf $(find $APPDIR  -maxdepth 1 -name '*.xml') "$cfgtmp"
}

function cfgrestore()
{
	if [ -d "$cfgtmp" ]
	then
		echo "restore config files"
		cp -vf $(find $cfgtmp -name '*.xml') "$APPDIR"
		rm -rf "$cfgtmp"
	fi
}

#++++++++++++++++main++++++++++++++++++++++
# user check; must be root
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "You must be root !"
	exit -1
fi

APPDIR="/usr/local/AgentService/"
APP_UNINSTALL="${APPDIR}uninstall.sh"
APP_AGENT_CONFIG="${APPDIR}agent_config.xml"
if [ -d ${APPDIR} ]; then
	if [ -f ${APP_AGENT_CONFIG} ]; then
		curversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $APP_AGENT_CONFIG)
	fi

	cfgbackup

	if  [ -f ${APP_UNINSTALL} ]; then	
		echo "Found RMM agent already installed. Remove and install the new ..."
		chmod +x "${APP_UNINSTALL}"
		${APP_UNINSTALL}
	fi
fi

# Collect new config file information
INSTALL_AGENT_CONFIG="${CURDIR}/AgentService/agent_config.xml"
if [ -f ${INSTALL_AGENT_CONFIG} ]; then
	servicename=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $INSTALL_AGENT_CONFIG)
	newversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $INSTALL_AGENT_CONFIG)
	serverport=$(sed -n 's:.*<ServerPort>\(.*\)</ServerPort>.*:\1:p' $INSTALL_AGENT_CONFIG)
else
	echo "not found: ${INSTALL_AGENT_CONFIG}"
fi

# Copy Agent Service
echo "Copy AgentService to /usr/local."
cp -avr ./AgentService /usr/local

cfgrestore;

# Overwrite Server Port and Service Name
if [[ $curversion == 3.0.* ]]; then
	if [[ $newversion == 3.1.* ]]; then
		echo "Overwrite Server Port"
		sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$serverport\2|" $APP_AGENT_CONFIG
		sed -i "s|\(<ServiceName>\)[^<>]*\(</ServiceName>\)|\1$servicename\2|" $APP_AGENT_CONFIG
	fi
fi

# Install SAWatchdog service
echo "Install SAWatchdog service"
cp ./sawatchdog /etc/init.d
chown root:root /etc/init.d/sawatchdog
chmod 0750 /etc/init.d/sawatchdog
/sbin/chkconfig --add sawatchdog

# Install SAWebsockify service
echo "Install SAWebsockify service"
cp ./sawebsockify /etc/init.d
chown root:root /etc/init.d/sawebsockify
chmod 0750 /etc/init.d/sawebsockify
/sbin/chkconfig --add sawebsockify

# Install SAAgent service
echo "Install SAAgent service"
cp ./saagent /etc/init.d
chown root:root /etc/init.d/saagent
chmod 0750 /etc/init.d/saagent
/sbin/chkconfig --add saagent

if [ -d "/etc/profile.d/" ]; then
	cp ./AgentService/xhostshare.sh "/etc/profile.d/"
	chmod 0644 /etc/profile.d/xhostshare.sh
fi

# pre-install 
PRE_INSTALL_FILE="/usr/local/AgentService/pre-install_chk.sh"
if [ -f "${PRE_INSTALL_FILE}" ]; then
	chmod +x $PRE_INSTALL_FILE
	$PRE_INSTALL_FILE
fi


SETUP_FILE="/usr/local/AgentService/setup.sh"
read -t 10 -p "Setup Agent [y/n](default:n): " ans
if [ "$ans" == "y" ]; then
    chmod +x $SETUP_FILE
	$SETUP_FILE
else
	echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
	echo "Start RMM Agent."
	# Start SAWatchdog    #echo "Start RMM SAWatchdog daemon..."
	/etc/init.d/sawatchdog start

	# Start RMM Agent    #echo "Start RMM Agent daemon..."
	/etc/init.d/saagent start
		
	# Start SAWebsockify  #echo "Start RMM SAWebsockify daemon..."
	/etc/init.d/sawebsockify start
fi
