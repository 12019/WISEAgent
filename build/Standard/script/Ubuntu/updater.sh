#!/bin/bash

#====== Some variables that you need modify ======# 
# Service operation and configure path
SAAGENT_START="start saagent"
SAAGENT_STOP="stop saagent"
SAWATCHDOG_START="start sawatchdog"
SAWATCHDOG_STOP="stop sawatchdog"
SAWEBSOCKIFY_START="start sawebsockify"
SAWEBSOCKIFY_STOP="stop sawebsockify"
srvInitPath="/etc/init/"

# Default value
appName="cagent"
appInstallPath="/usr/local/AgentService"
PRE_INSTALL_FILE="${appInstallPath}/pre-install_chk.sh"
SETUP_FILE="${appInstallPath}/setup.sh"

#====== Some function that you need modify ======#
setxhost()
{	
	local destDir="/etc/profile.d/"
	echo -n "xhost set:"
	if [ -d "$destDir" ]; then
		cp -vf ./AgentService/xhostshare.sh $destDir
		chmod a+x ${destDir}xhostshare.sh
		${destDir}xhostshare.sh
	else
		echo "The folder $destDir is not exist!"
	fi
}

configService()
{
    # Install SAWatchdog service
    cp ./sawatchdog $srvInitPath
    chown root:root ${srvInitPath}sawatchdog
    chmod 0644 ${srvInitPath}sawatchdog
    mv ${srvInitPath}sawatchdog 	${srvInitPath}sawatchdog.conf

    # Install SAAgent service
	cp ./saagent $srvInitPath
	chown root:root ${srvInitPath}saagent
	chmod 0644 ${srvInitPath}saagent
	mv ${srvInitPath}saagent 	${srvInitPath}saagent.conf
	
	 # Install SAWebsockify service
	cp ./sawebsockify $srvInitPath
	chown root:root ${srvInitPath}sawebsockify
	chmod 0644 ${srvInitPath}sawebsockify
	mv ${srvInitPath}sawebsockify 	${srvInitPath}sawebsockify.conf
}

keepUserCfg()
{	
	if [ -d ${appInstallPath} ]; then
		local userCfgFile=$(find ${appInstallPath} -maxdepth 1 -name '*.xml')
		local oldVernSrvName=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $userCfgFile 2>/dev/null)
		local oldversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $userCfgFile 2>/dev/null)
		local newVernSrvName=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' ./AgentService/agent_config.xml)
		local newserverport=$(sed -n 's:.*<ServerPort>\(.*\)</ServerPort>.*:\1:p' ./AgentService/agent_config.xml)
		local newversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' ./AgentService/agent_config.xml)
        
		#if [ ${oldVernSrvName} == ${newVernSrvName} ]; then 
			echo "Keep user configre:"
			cp -vf ${userCfgFile} ./AgentService
        	#else 
        		####-- not support like 3.0 --> 3.1 --####
			#echo "Keep user configre:" >/dev/null # Do nothing, just hold this place
        	#fi 
		
		# Overwrite Server Port and Service Name
		if [[ $curversion == 3.0.* ]]; then
			if [[ $newversion == 3.1.* ]]; then
				echo "Overwrite Server Port"
				sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$newserverport\2|" ./AgentService/agent_config.xml
				sed -i "s|\(<ServiceName>\)[^<>]*\(</ServiceName>\)|\1$newVernSrvName\2|" ./AgentService/agent_config.xml
			fi
		fi
	fi
}



#===================== main =====================#
# -- User check, must be root -- #
if [ $UID -gt 0 ] && [ "`id -un`" != "root" ]; then
	echo "Permission denied!" 
	exit -1
fi

# -- Install or Update RMM Agent -- #
agentcfgfile="${appInstallPath}/agent_config.xml"
servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $agentcfgfile  2>/dev/null)
agentpidfile="/var/run/${servername}.pid"
watchdogpidfile="/var/run/SAWatchdog.pid"


echo "Install RMM Agent:" 
echo "Current path: $(pwd)" 

# Ignore TERM signal
trap "" TERM 


# Stop service 
[ -f "$watchdogpidfile" ] && ${SAWATCHDOG_STOP}	             2>&1
[ -f "$agentpidfile" ] && ${SAAGENT_STOP}                    2>&1
[ -e "/etc/init/sawebsockify.conf" ] && ${SAWEBSOCKIFY_STOP} 2>&1

 

# Keep user configure 
keepUserCfg
# Remove old file 
[ -d "$appInstallPath" ] && rm -rf "$appInstallPath" && echo "Remove folder $appInstallPath" 
# Install new version
echo "Copy RMM Agent to /usr/local:"
cp -avr ./AgentService 	/usr/local

# pre-install 
if [ -f "${PRE_INSTALL_FILE}" ]; then
	chmod +x ${PRE_INSTALL_FILE} 
	${PRE_INSTALL_FILE}
fi
# Set xhost for Screenshot and KVM
setxhost

# Service configure
configService

# Setup 
read -t 10 -p "Setup Agent[y/n](default:n): " ans
if [ "$ans" == "y" ]; then
	chmod +x ${SETUP_FILE}
	${SETUP_FILE}
else
	# Start Service: SAWebsockify and SAWatchdog
	echo "Start RMM Agent:"
	${SAAGENT_START}
	${SAWEBSOCKIFY_START}
	${SAWATCHDOG_START}
	echo
	echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later."
	echo
fi



exit 0
