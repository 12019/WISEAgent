#!/bin/sh
daemon_saagent="/etc/init.d/saagent"
daemon_sawatchdog="/etc/init.d/sawatchdog"
daemon_sawebsockify="/etc/init.d/sawebsockify"

get_linux_arch ()
{
    b=`uname -i 2>/dev/null`
    # For some old linux'es, -i option is not supported with uname. We default
    # to X86 in such a case until we actually start supporting these distros on
    # AMD64.
    if [ $? -ne 0 ]; then
        host_arch="X86"
    else
        case $b in
        i386)  host_arch="X86";;
        amd64) host_arch="AMD64";;
        x86_64)host_arch="AMD64";;
        *)echo "Unsupported Architecture $b found on machine." ;;
        esac
    fi
}


pre_setup_actions ()
{
	echo "sending request to stop AgentService" 
	$daemon_sawatchdog   stop 2>&1
	$daemon_sawebsockify stop 2>&1
	$daemon_saagent      stop 2>&1
}


post_setup_actions ()
{
	# echo "sending request to start AgentService" 
	# $daemon_saagent      start 2>&1
	# $daemon_sawatchdog   start 2>&1
	# $daemon_sawebsockify start 2>&1
	local IsRunning=`$daemon_sawebsockify status 2>&1 | grep pid`
	if [ -z $IsRunning ]; then 
		$daemon_sawebsockify start
	else 
		$daemon_sawebsockify restart
	fi
	
	IsRunning=`$daemon_sawatchdog status 2>&1 | grep pid`
	if [ -z $IsRunning ]; then
		$daemon_sawatchdog start
	else
		$daemon_sawatchdog restart
	fi
}


check_linux_version ()
{
	if [ -f "/etc/redhat-release" ]; then
		RELEASE_PATH="/etc/redhat-release"
	elif [ -f "/etc/SuSE-release" ]; then
		RELEASE_PATH="/etc/SuSE-release"
	fi
	get_linux_arch
	SOFTWARE_ID=SOLIDCOR610-9500_LNX
	BUILD_NUMBER=6.1.0-9500
	AS3="Red Hat Enterprise Linux AS release 3"
	ES3="Red Hat Enterprise Linux ES release 3"
	WS3="Red Hat Enterprise Linux WS release 3"
	AS4="Red Hat Enterprise Linux AS release 4"
	ES4="Red Hat Enterprise Linux ES release 4"
	WS4="Red Hat Enterprise Linux WS release 4"
	CS4="CentOS release 4"
	EL5="Red Hat Enterprise Linux Server release 5"
	OEL5="Enterprise Linux Enterprise Linux Server release 5"
	CS5="CentOS release 5"
	EL6="Red Hat Enterprise Linux Server release 6"
	CS6="CentOS release 6"
  	SES9="SUSE LINUX Enterprise Server 9"
	SES9x="SuSE Linux 9"
	SES10="SUSE Linux Enterprise Server 10"
	SED11="SUSE Linux Enterprise Desktop 11"
	SES11="SUSE Linux Enterprise Server 11"
	Opensuse10="openSUSE 10"
	Opensuse11="openSUSE 11"
	Opensuse12="openSUSE 12"
	LRH72="Red Hat Linux release 7"
	LINUX_REL=`cat $RELEASE_PATH | head -n 1 | awk -F " [(]" '{ print $1 }' | awk -F "." '{ print $1 }'`
	case $LINUX_REL in
	$AS3)host_distro="LEL3";;           	# Redhat AS 3
	$AS4)host_distro="LEL4";;           	# Redhat AS 4
	$ES3)host_distro="LEL3";;           	# Redhat ES 3
	$ES4)host_distro="LEL4";;           	# Redhat ES 4
	$WS3)host_distro="LEL3";;           	# Redhat WS 3
	$WS4)host_distro="LEL4";;           	# Redhat WS 4
	$EL5)host_distro="LEL5";;           	# Redhat EL 5
	$OEL5)host_distro="LEL5";;          	# Oracle EL 5
	$CS4)host_distro="LEL4";;           	# CentOS 4
	$CS5)host_distro="LEL5";;           	# CentOS 5
	$EL6)host_distro="LEL6";;           	# Redhat EL 6
	$CS6)host_distro="LEL6";;           	# CentOS 6
	$SES9)host_distro="LSES9";;         	# SLES 9
	$SES9x)host_distro="LSES9";;	    	# Suse 9.3 Pro
	$SES10)host_distro="LSES10";;       	# SLES 10
	$SED11)host_distro="LSES11";;       	# SLED 11
	$SES11)host_distro="LSES11";;       	# SLES 11
	$Opensuse10)host_distro="LSES10";;	    # OpenSuse 10
	$Opensuse11)host_distro="LSES11";;	    # OpenSuse 11
	$Opensuse12)host_distro="LSES12";;	    # OpenSuse 12
	$LRH72)host_distro="LRH72";;		    # Redhat 7.2
	*)host_distro="non"
	esac
        Linux_RH=`uname -a | grep xen 2>/dev/null`
	if [ "$Linux_RH" != "" ]; then
	    host_distro="non"
	fi
        arch=`uname -i 2>/dev/null`
	if [ $? -ne 0 ]; then
	    arch="X86"
	fi
	kver=`uname -r 2>/dev/null`
}


update_config()
{
	sed -i "s|\(<ServerIP>\)[^<>]*\(</ServerIP>\)|\1$serveraddress\2|" $filename
	sed -i "s|<ServerIP/>|<ServerIP>$serveraddress</ServerIP>|g" $filename
	
	sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$serverport\2|" $filename
	sed -i "s|<ServerPort/>|<ServerPort>$serverport</ServerPort>|g" $filename
	
	if [ -n "$username" ]; then
		sed -i "s|\(<UserName>\)[^<>]*\(</UserName>\)|\1$username\2|" $filename
		sed -i "s|<UserName/>|<UserName>$username</UserName>|g" $filename
		
		sed -i "s|\(<UserPassword>\)[^<>]*\(</UserPassword>\)|\1$userpassword\2|" $filename
		sed -i "s|<UserPassword/>|<UserPassword>$userpassword</UserPassword>|g" $filename
	fi
	
	if [ -n "$serveraddress" ]; then
		sed -i "s|\(<RunMode>\)[^<>]*\(</RunMode>\)|\1Remote\2|" $filename
		sed -i "s|<RunMode/>|<RunMode>Remote</RunMode>|g" $filename
	else
		sed -i "s|\(<RunMode>\)[^<>]*\(</RunMode>\)|\1Standalone\2|" $filename
		sed -i "s|<RunMode/>|<RunMode>Standalone</RunMode>|g" $filename
	fi
	sed -i "s|\(<DeviceName>\)[^<>]*\(</DeviceName>\)|\1$devicename\2|" $filename
	sed -i "s|<DeviceName/>|<DeviceName>$devicename</DeviceName>|g" $filename
	
	sed -i "s|\(<AmtID>\)[^<>]*\(</AmtID>\)|\1$amtid\2|" $filename
	sed -i "s|<AmtID/>|<AmtID>$amtid</AmtID>|g" $filename
	
	sed -i "s|\(<AmtPwd>\)[^<>]*\(</AmtPwd>\)|\1$amtpassword\2|" $filename
	sed -i "s|<AmtPwd/>|<AmtPwd>$amtpassword</AmtPwd>|g" $filename
	
	if [ -n "$amtid" ]; then
		if [ -n "$amtpassword" ]; then
			sed -i "s|\(<AmtEn>\)[^<>]*\(</AmtEn>\)|\1True\2|" $filename
			sed -i "s|<AmtEn/>|<AmtEn>True</AmtEn>|g" $filename
		else
			sed -i "s|\(<AmtEn>\)[^<>]*\(</AmtEn>\)|\1False\2|" $filename
			sed -i "s|<AmtEn/>|<AmtEn>False</AmtEn>|g" $filename
		fi
	else
		sed -i "s|\(<AmtEn>\)[^<>]*\(</AmtEn>\)|\1False\2|" $filename
		sed -i "s|<AmtEn/>|<AmtEn>False</AmtEn>|g" $filename
	fi
	
	if [ $vnctype == 1 ]; then
		sed -i "s|\(<KVMMode>\)[^<>]*\(</KVMMode>\)|\1customvnc\2|" $filename
		sed -i "s|<KVMMode/>|<KVMMode>customvnc</KVMMode>|g" $filename
	elif [ $vnctype == 2 ]; then
		sed -i "s|\(<KVMMode>\)[^<>]*\(</KVMMode>\)|\1disable\2|" $filename
		sed -i "s|<KVMMode/>|<KVMMode>disable</KVMMode>|g" $filename
	else
		sed -i "s|\(<KVMMode>\)[^<>]*\(</KVMMode>\)|\1default\2|" $filename
		sed -i "s|<KVMMode/>|<KVMMode>default</KVMMode>|g" $filename
	fi
	
	sed -i "s|\(<CustVNCPort>\)[^<>]*\(</CustVNCPort>\)|\1$vncportnum\2|" $filename
	sed -i "s|<CustVNCPort/>|<CustVNCPort>$vncportnum</CustVNCPort>|g" $filename
	
	sed -i "s|\(<CustVNCPwd>\)[^<>]*\(</CustVNCPwd>\)|\1$vncpassword\2|" $filename
	sed -i "s|<CustVNCPwd/>|<CustVNCPwd>$vncpassword</CustVNCPwd>|g" $filename
	
	if [ "$autoreconnect" == "" ]; then
		sed -i "s|\(<AutoReconnect>\)[^<>]*\(</AutoReconnect>\)|\1True\2|" $filename
		sed -i "s|<AutoReconnect/>|<AutoReconnect>True</AutoReconnect>|g" $filename
	elif [ "$autoreconnect" == "enable" ]; then
		sed -i "s|\(<AutoReconnect>\)[^<>]*\(</AutoReconnect>\)|\1True\2|" $filename
		sed -i "s|<AutoReconnect/>|<AutoReconnect>True</AutoReconnect>|g" $filename
	elif [ "$autoreconnect" == "disable" ]; then
		sed -i "s|\(<AutoReconnect>\)[^<>]*\(</AutoReconnect>\)|\1False\2|" $filename
		sed -i "s|<AutoReconnect/>|<AutoReconnect>False</AutoReconnect>|g" $filename
        else
               echo "Input error"
	fi

	sed -i "s|\(<TLSType>\)[^<>]*\(</TLSType>\)|\1$ssltype\2|" $filename
	sed -i "s|<TLSType/>|<TLSType>$ssltype</TLSType>|g" $filename

	sed -i "s|\(<CAFile>\)[^<>]*\(</CAFile>\)|\1$sslcafile\2|" $filename
	sed -i "s|<CAFile/>|<CAFile>$sslcafile</CAFile>|g" $filename

	sed -i "s|\(<CAPath>\)[^<>]*\(</CAPath>\)|\1$sslcapath\2|" $filename
	sed -i "s|<CAPath/>|<CAPath>$sslcapath</CAPath>|g" $filename

	sed -i "s|\(<CertFile>\)[^<>]*\(</CertFile>\)|\1$sslcertfile\2|" $filename
	sed -i "s|<CertFile/>|<CertFile>$sslcertfile</CertFile>|g" $filename

	sed -i "s|\(<KeyFile>\)[^<>]*\(</KeyFile>\)|\1$sslkeyfile\2|" $filename
	sed -i "s|<KeyFile/>|<KeyFile>$sslkeyfile</KeyFile>|g" $filename

	sed -i "s|\(<CertPW>\)[^<>]*\(</CertPW>\)|\1$sslcertpw\2|" $filename
	sed -i "s|<CertPW/>|<CertPW>$sslcertpw</CertPW>|g" $filename

	sed -i "s|\(<PSK>\)[^<>]*\(</PSK>\)|\1$sslpskey\2|" $filename
	sed -i "s|<PSK/>|<PSK>$sslpskey</PSK>|g" $filename

	sed -i "s|\(<PSKIdentify>\)[^<>]*\(</PSKIdentify>\)|\1$sslpskid\2|" $filename
	sed -i "s|<PSKIdentify/>|<PSKIdentify>$sslpskid</PSKIdentify>|g" $filename

	sed -i "s|\(<PSKCiphers>\)[^<>]*\(</PSKCiphers>\)|\1$sslpskcipher\2|" $filename
	sed -i "s|<PSKCiphers/>|<PSKCiphers>$sslpskcipher</PSKCiphers>|g" $filename
}


filename=/usr/local/AgentService/agent_config.xml
encrypttool=AgentEncrypt


config_susiaccess ()
{
	serverport=$port
	username=$usrname
	userpassword=$usrpass
	ssltype=$tlstype
	sslcafile=$cafile
	sslcapath=$capath
	sslcertfile=$certfile
	sslkeyfile=$keyfile
	sslcertpw=$certpw
	sslpskey=$pskey
	sslpskid=$pskid
	sslpskcipher=$pskcipher
	devicename=$name
	amtid=$amt
	amtpassword=$amtpass
	vnctype=$vncmode
	vncportnum=$vnc
	vncpassword=$vncpass
	autoreconnect=$reconn
	enssl="n"
	if [ $ssltype == 0 ]; then
		enssl="n"
	else
		enssl="y"
	fi

        echo "****************************************************************************************************"
	read -p "Do you want to configure RMM Agent now? [y/n](default: y)" susiaccess_config_ans
	if [ "$susiaccess_config_ans" == "y" ] ||[ "$susiaccess_config_ans" == "Y"  ] ||[ "$susiaccess_config_ans" == ""  ]; then
				read -p "Input Server IPAddress(default:$address): " serveraddress
				if [ "$serveraddress" == "" ]; then
					serveraddress=$address
				fi	
								
				while true
		                do
					read -p "Input Server Port(default:$port): " serverport
					
					if [ "$serverport" == "" ]; then
						serverport=$port
						break
					elif [[ $serverport =~ ^[0-9]+$ ]] && [ $serverport -le 65535 ] && [ $serverport -ge 1 ]; then
						break
					else
						echo "Input Server Port Number is error!"
					fi
		                done	
				
				if [ "$usrname" == "" ]; then
					usrname="anonymous"
				fi

#				while true
#		                do
#					re="^[A-Za-z0-9]{4,128}$"
#					read -p "Input Login Name[Len:4--35](default:$usrname): " username
					if [ "$username" == "" ]; then
						username=$usrname
#						break
#					elif [[ $username =~ $re ]]; then 
#						break
#					else
#						echo "Input error string"
					fi
#		                done	
					
#				while true
#		                do
#					re="^[A-Za-z0-9]{4,128}$"
#					unset userpassword
#					unset CHARCOUNT
#					CHARCOUNT=0
#					prompt="Input Login Password[Len:4--35, or na](default:$usrpass): "
#					while IFS= read -p "$prompt" -r -s -n 1 char
#					do
#						# Enter - accept password
#						if [[ $char == $'\0' ]] ; then
#							break
#						fi
#						# Backspace
#						if [[ $char == $'\177' ]] ; then
#							if [ $CHARCOUNT -gt 0 ] ; then
#								CHARCOUNT=$((CHARCOUNT-1))
#								prompt=$'\b \b'
#								amtpassword="${amtpassword%?}"
#							else
#								prompt=''
#							fi
#						else
#							CHARCOUNT=$((CHARCOUNT+1))
#							prompt='*'
#							amtpassword+="$char"
#						fi
#					done
#					echo
#					#read -p "Input Login Password[Len:4--35, or na](default:$usrpass): " -s userpassword
					if [ "$userpassword" == "" ]; then
						userpassword=$usrpass
#						break
#					elif [ "$userpassword" == "na" ]; then 
#						userpassword=""
#						break
#					elif [[ $userpassword =~ $re ]]; then 
#						userpassword=$(./$encrypttool $userpassword)
#						break
#					else
#						echo "Input error string"
					fi
#		                done	
				
#				while true
#		                do
#			                read -p "Select TLS Mode[0:Disable, 1:Certificates file, 2:Pre-shared Key](default:$tlstype): " ssltype
#					if [ "$ssltype" == "" ]; then
#						ssltype=$tlstype
#				        	break
#				    	elif [[ $ssltype =~ ^[0-2]?$ ]]; then
#						break
#					else
#						echo "Input TLS Mode is error!"
#					fi
#				done
				

				while true
		                do
					read -p "Enalbe TLS [y/n](default: $enssl): " ssl
					if [ "$ssl" == ""  ]; then
						if [ "$enssl" == "n"  ]; then
							ssltype=0
						else
							ssltype=$tlstype
						fi
						break
					elif [ "$ssl" == "y" ] || [ "$ssl" == "Y"  ]; then
						ssltype=2
						break
					elif [ "$ssl" == "n" ] || [ "$ssl" == "N"  ] || [ "$ssl" == ""  ]; then
						ssltype=0
						break
					else
						echo "Input error string"
					fi
				done

########################################(Device Name)############################################################		
		                while true
		                do
					read -p "Input Device Name[Len:4--35](default:$name): " devicename
					if [ "$devicename" == "" ]; then
						devicename=$name						
					fi
					len=${#devicename}
			                if [ $len -ge 4 ] && [ $len -le 35 ]; then
				               break
			                else
				               echo "Input error string"
			                fi
		                done
########################################(AMT Config)############################################################ 
				while true
		                do
					re="^[A-Za-z0-9]{4,35}$"
					read -p "Input AMT ID[Len:4--35, or na](default:$amt): " amtid
					if [ "$amtid" == "" ]; then
						amtid=$amt
						break
					elif [ "$amtid" == "na" ]; then
						amtid=""
						break
					elif [[ $amtid =~ $re ]]; then 
						break
					else
						echo "Input error string"
					fi
		                done	
		                
				while true
		                do
					re="^[A-Za-z0-9~!@#$%^&*()_+|.]{8,16}$"
					unset amtpassword
					unset CHARCOUNT
					CHARCOUNT=0
					prompt="Input AMT password[Len:8--16, or na](default:$amtpass): "
					while IFS= read -p "$prompt" -r -s -n 1 char
					do
						# Enter - accept password
						if [[ $char == $'\0' ]] ; then
							break
						fi
						# Backspace
						if [[ $char == $'\177' ]] ; then
							if [ $CHARCOUNT -gt 0 ] ; then
								CHARCOUNT=$((CHARCOUNT-1))
								prompt=$'\b \b'
								amtpassword="${amtpassword%?}"
							else
								prompt=''
							fi
						else
							CHARCOUNT=$((CHARCOUNT+1))
							prompt='*'
							amtpassword+="$char"
						fi
					done
					echo
					#read -p "Input AMT password[Len:8--16, or na](default:$amtpass): " -s amtpassword
					if [ "$amtpassword" == "" ]; then
						amtpassword=$amtpass
						break
					elif [ "$amtpassword" == "na" ]; then 
						amtpassword=""
						break
					elif [[ $amtpassword =~ $re ]]; then 
						amtpassword=$(./$encrypttool $amtpassword)
						break
					else
						echo "Input error string"
					fi
		                done	
################################################################################################################
		                swversion=$version
###########################################(VNC Port Config)####################################################
				while true
		                do
			                read -p "Select KVM Mode[0:default, 1:custom VNC, 2:disable](default:$vncmode): " vnctype
					if [ "$vnctype" == "" ]; then
						vnctype=$vncmode
				        break
				    elif [[ $vnctype =~ ^[0-2]?$ ]]; then
						break
					else
						echo "Input KVM Mode is error!"
					fi
				done
				
		                while true
		                do
			                read -p "Input VNC Port[1--65535](default :$vnc): " vncportnum
			                if [ "$vncportnum" == ""  ]; then
				               vncportnum=$vnc
				               break
				        elif [[ $vncportnum =~ ^[0-9]+$ ]] && [ $vncportnum -le 65535 ] && [ $vncportnum -ge 1 ]; then
						break
					else
						echo "Input VNC Port Number is error!"
					fi
				done
				if [ $vnctype == 1 ]; then
					while true
					do
						re="^[A-Za-z0-9]{4,35}$"
						unset vncpassword
						unset CHARCOUNT
					        CHARCOUNT=0
						prompt="Input VNC password[Len:4--35](default:$vncpass): "
						while IFS= read -p "$prompt" -r -s -n 1 char
						do
							# Enter - accept password
							if [[ $char == $'\0' ]] ; then
								break
							fi
							# Backspace
							if [[ $char == $'\177' ]] ; then
								if [ $CHARCOUNT -gt 0 ] ; then
									CHARCOUNT=$((CHARCOUNT-1))
									prompt=$'\b \b'
									vncpassword="${vncpassword%?}"
								else
									prompt=''
								fi
							else
								CHARCOUNT=$((CHARCOUNT+1))
								prompt='*'
								vncpassword+="$char"
							fi
						done
						echo
						#read -p "Input VNC password[Len:4--35](default:$vncpass): " -s vncpassword
						if [ "$vncpassword" == "" ]; then
							vncpassword=$vncpass
							break
						elif [[ $vncpassword =~ $re ]]; then 
							vncpassword=$(./$encrypttool $vncpassword)
							break
						else
							echo "Input error string"
						fi
					done	
				fi
				
################################################################################################################
				while true
				do
					read -p "Enable auto reconnect[enable/disable](default: $reconn): " autoreconnect
					if [ "$autoreconnect" == "" ]; then
						autoreconnect=$reconn
						break
					elif [ "$autoreconnect" == "enable" ]; then
						autoreconnect="enable"
						reconn="enable"
						break
					elif [ "$autoreconnect" == "disable" ]; then
						autoreconnect="disable"
						reconn="disable"
						break
					else
						echo "Input error"
					fi
				done
################################################################################################################
#		if [ -f $filename ]; then
#		        cp -f $filename $filename.bak
#		fi
				update_config					
################################################################################################################
	elif [ "$susiaccess_config_ans" == "n" ] ||[ "$susiaccess_config_ans" == "N"  ]; then
		#update_config
		echo "You can execute setup.sh to set it later."
		#exit
	else
	    #update_config
		echo "Invalid answer. You can execute setup.sh to set it later."
		exit
	fi
	
#	if [ $reconn == "disable" ]; then
		echo "****************************************************************************************************"
		read -p "Do you want to start RMM Agent now? [y/n](default: y)" susiaccess_start_ans
		if [ "$susiaccess_start_ans" == "y" ] ||[ "$susiaccess_start_ans" == "Y"  ] ||[ "$susiaccess_start_ans" == ""  ]; then
			local IsRunning=`$daemon_saagent status 2>&1 | grep pid`
			if [ -z $IsRunning ]; then 
				$daemon_saagent start
			else 
				$daemon_saagent restart
			fi
			
			# $daemon_saagent status >/dev/null 2>&1
			# if [ $? -ne 0 ]; then
				# #echo -e "\nRMM Agent Service Starting..." 
				# $daemon_saagent start
			# else
				# #echo -e "\nRMM Agent Service Restarting..." 
				# $daemon_saagent restart
			# fi		
		fi
#	fi
}
address=""
port=1883
name=$HOSTNAME
amt=""
amtpass=""
version=Linux-2.1.dateversion-svnversion
vnc=10000
vncmode=0
vncpass=""
tlstype=0
cafile=""
capath=""
certfile=""
keyfile=""
certpw=""
pskey="05155853"
pskid=""
pskcipher=""
reconn="enable"


load_config()
{
	address=$(sed -n 's:.*<ServerIP>\(.*\)</ServerIP>.*:\1:p' $filename)
	port=$(sed -n 's:.*<ServerPort>\(.*\)</ServerPort>.*:\1:p' $filename)
	name=$(sed -n 's:.*<DeviceName>\(.*\)</DeviceName>.*:\1:p' $filename)
	if [ "$name" == "" ]; then
		name=$HOSTNAME
	fi
	amt=$(sed -n 's:.*<AmtID>\(.*\)</AmtID>.*:\1:p' $filename)
	amtpass=$(sed -n 's:.*<AmtPwd>\(.*\)</AmtPwd>.*:\1:p' $filename)
	mode=$(sed -n 's:.*<KVMMode>\(.*\)</KVMMode>.*:\1:p' $filename)
	if [ "$mode" == "customvnc" ]; then
		vncmode=1
	elif [ "$mode" == "disable" ]; then
		vncmode=2
	else
		vncmode=0
	fi
	vnc=$(sed -n 's:.*<CustVNCPort>\(.*\)</CustVNCPort>.*:\1:p' $filename)
	vncpass=$(sed -n 's:.*<CustVNCPwd>\(.*\)</CustVNCPwd>.*:\1:p' $filename)
	auto=$(sed -n 's:.*<AutoReconnect>\(.*\)</AutoReconnect>.*:\1:p' $filename)
	if [ "$auto" == "False" ]; then
		reconn="disable"
        else
		reconn="enable"
	fi
	
	usrname=$(sed -n 's:.*<UserName>\(.*\)</UserName>.*:\1:p' $filename)
	usrpass=$(sed -n 's:.*<UserPassword>\(.*\)</UserPassword>.*:\1:p' $filename)
       
        tlstype=$(sed -n 's:.*<TLSType>\(.*\)</TLSType>.*:\1:p' $filename)
	cafile=$(sed -n 's:.*<CAFile>\(.*\)</CAFile>.*:\1:p' $filename)
	capath=$(sed -n 's:.*<CAPath>\(.*\)</CAPath>.*:\1:p' $filename)
	certfile=$(sed -n 's:.*<CertFile>\(.*\)</CertFile>.*:\1:p' $filename)
	keyfile=$(sed -n 's:.*<KeyFile>\(.*\)</KeyFile>.*:\1:p' $filename)
	certpw=$(sed -n 's:.*<CertPW>\(.*\)</CertPW>.*:\1:p' $filename)
	pskey=$(sed -n 's:.*<PSK>\(.*\)</PSK>.*:\1:p' $filename)
	pskid=$(sed -n 's:.*<PSKIdentify>\(.*\)</PSKIdentify>.*:\1:p' $filename)
	pskcipher=$(sed -n 's:.*<PSKCiphers>\(.*\)</PSKCiphers>.*:\1:p' $filename)
}



check_selinux ()
{
	if [ -f /etc/selinux/config ]; then
		selinux=`/usr/sbin/sestatus | awk -F "[: ]+" '{print $3}' | sed -n '1p'`
		if [ "$selinux" = "enabled" ]; then
			echo "Warning SELinux is enabled, Installer can automaticly help you to disable SELinux."
			#read -p "Do you want to disable it? [y/n](default: y)" selinux_ans
			#if [ "$selinux_ans" == "y" ] ||[ "$selinux_ans" == "Y"  ] ||[ "$selinux_ans" == ""  ]; then
				sed -i '/SELINUX/s/enforcing/disabled/' /etc/selinux/config
				sed -i '/SELINUX/s/permissive/disabled/' /etc/selinux/config
				echo "****************************************************************************************************"
				echo "Selinux is disabled, you need reboot now and run './setup.sh' again!!!"   
				echo "****************************************************************************************************"
				read -p "Do you want to reboot now? [y/n](default: n)" reboot_ans
				if [ "$reboot_ans" == "y" ]; then
					reboot
				fi
			#fi
			exit
		fi
	fi
}



check_firewall()
{
	if [ -f "/etc/SuSE-release" ]; then
		/sbin/SuSEfirewall2 stop >/dev/null 2>&1
		/sbin/SuSEfirewall2 off >/dev/null 2>&1
	else
		/etc/init.d/iptables stop >/dev/null 2>&1
		/sbin/chkconfig iptables off >/dev/null 2>&1
	fi
	echo "****************************************************************************************************"
	echo "FireWall is disabled"	
	#echo "****************************************************"
}



check_susiaccess()
{
	servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $filename)
	path="/usr/local/AgentService"
	file="/var/run/$servername.pid"
	if [ -f "$file" ]; then
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
	if [ -d "$path" ]; then
		pre_setup_actions
		config_susiaccess
		post_setup_actions
	else
		echo "AgentService not found!"
	fi
}



THIS_PLATFORM="`uname`"
baseDirForScriptSelf=$(cd "$(dirname "$0")"; pwd)
rm -f $DEBUG_INSTALL_LOG
host_arch=
firstpara=""


#===========================================Main=======================================
echo -e "========================== AgentService Linux Setup =========================="
firstpara=$1
numpara=$#
if [ $# -ne 0 ] &&[ "$1" != "-f" ]; then
	echo " Parameter incorrect!"
	exit
fi
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "Permission denied!"
	exit
fi
if [ "$THIS_PLATFORM" != "Linux" ]; then
	echo "This RMM Agent only support Linux!"
	exit
fi 
filesystem_type=`mount | grep / -w | awk '{print $5}'`

if [ -f "$encrypttool" ]; then
	encrypttool="AgentEncrypt"
else
	encrypttool="AgentService/AgentEncrypt"
fi

check_firewall
check_selinux
check_linux_version
load_config
check_susiaccess
echo "RMM Agent Linux setup successfully!"


