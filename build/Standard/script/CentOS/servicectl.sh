#!/bin/bash
NAME=$1
CMA_SCRIPT=/etc/init.d/$NAME

agent_stop ()
{
	# echo "sending request to stop $NAME" 
	$CMA_SCRIPT stop  2>&1
}

agent_start ()
{
	# echo "sending request to start $NAME" 
	$CMA_SCRIPT start  2>&1
}

agent_status ()
{
	# echo "sending request to get $NAME status" 
	$CMA_SCRIPT status 2>&1
}

if [ "$2" == "start" ]; then
	agent_start;
elif [ "$2" == "stop" ]; then
	agent_stop;
else
	agent_status;
fi
