#!/bin/bash
NAME=$1

agent_stop ()
{
	# echo "sending request to stop $NAME" 
	stop $NAME  2>&1
}

agent_start ()
{
	# echo "sending request to start $NAME" 
	start $NAME  2>&1
}

agent_status ()
{
	# echo "sending request to get $NAME status" 
	status $NAME  2>&1
}

if [ "$2" == "start" ]; then
	agent_start;
elif [ "$2" == "stop" ]; then
	agent_stop;
else
	agent_status;
fi
