#!/bin/sh

#
# Author:AlexChen
# Date:2015/09/02
# Company:Shenzhen Ucpaas technology co., LTD
#
ulimit -c unlimited
PATH=.:$PATH
export PATH
export scriptpath=$(pwd)
export exe=smsp_c2s
export binpath=$scriptpath/bin/$exe

# print help
function print_help()
{
	echo -e "<<# # # - - - - - - - - - - - help - - - - - - - - - - - # # #"
	echo -e "-start				./start.sh -start"
	echo -e "-stop				./start.sh -stop"
	echo -e "-restart			./start.sh -restart"
	echo -e "-ps				./start.sh -ps"
	echo -e "<<# # # - - - - - - - - - - - help - - - - - - - - - - - # # #"
}

## start server
function start_svr()
{
	if [ ! -f "$binpath" ]; then 
		echo "- [start_svr:$LINENO] $binpath is not exist!" 
		echo "- [start_svr:$LINENO] start the $binpath is failed!" 
		return
	fi
	
	check_process $binpath
	if [ $? -eq 1 ]; then
		echo "- [start_svr:$LINENO] start the $binpath is failed!" 
		return
	fi
	nohup $binpath > $exe.log 2>&1 &
	if [ $? -eq 1 ]; then
		echo "- [start_svr:$LINENO] start the $binpath is failed!" 
	else
		echo "+ [start_svr:$LINENO] start the $binpath is success!" 
	fi
}

## kill server
function kill_svr()
{
	var=`ps aux | grep -i "$1" | grep -v grep | awk '{print $2}'`
	OLD_IFS="$IFS" 
	IFS=" " 
	array=($var) 
	IFS="$OLD_IFS"
	
	if [ ${#array} -eq 0 ]; then
		echo "- [kill_svr:$LINENO] pid is NULL!" 
		return 1
	fi
	
	for s in ${array[@]} 
	do 
		kill $s
		echo "+ [kill_svr:$LINENO] kill $s" 
	done
	
	return 0
}

## stop server
function stop_svr()
{
	if [ ! -f "$binpath" ]; then 
		echo "- [stop_svr:$LINENO] $binpath is not exist!" 
		echo "- [stop_svr:$LINENO] kill the $binpath is failed!" 
		return
	fi
	
	kill_svr $binpath
	if [ $? -eq 1 ]; then
		echo "- [stop_svr:$LINENO] kill the $binpath is failed!" 
	else
		echo "+ [stop_svr:$LINENO] kill the $binpath is success!" 
	fi
}

## ps -ef|grep svr
function ps_svr()
{
	if [ ! $1 ] ; then
		ps -ef|grep $exe$
		return
	fi
	
	ps -ef|grep $1
}

## check server
function check_process()
{
	var=`ps aux | grep -i "$1" | grep -v grep | awk '{print $2}'`
	OLD_IFS="$IFS" 
	IFS=" " 
	array=($var) 
	IFS="$OLD_IFS"
	
	if [ ${#array} -eq 0 ]; then
		return 0
	else
		echo "- [check_process:$LINENO] $binpath is exist!" 
		return 1
	fi
	
}

## restart server
function restart_svr()
{
	stop_svr && sleep 1 && start_svr
}

function main()
{
	if [ ! $1 ] ; then
		start_svr
		return
	fi
	
	case $1 in
		-start)
			start_svr
			;;
		-stop)
			stop_svr
			;;
		-restart)
			restart_svr
			;;
		-ps)
			ps_svr $2
			;;
		-help)
			print_help
			;;
		*)
			print_help
			;;
	esac

	exit 0
}

main $@
