#!/bin/sh

#
# Author:AlexChen
# Date:2017/8/23
# Company:Shenzhen Ucpaas technology co., LTD
#

####
#
# Please put the modified file under SMSP
# smsp---
#     smsp_c2s ---
#     smsp_http---
#     startall.sh
####


ulimit -c unlimited
PATH=.:$PATH
export PATH
export scriptpath=$(pwd)
export exefilelist=(smsp_access smsp_audit smsp_c2s smsp_charge smsp_consumer smsp_consumer_access smsp_consumer_record smsp_http smsp_report smsp_send)
export exepathlist=()
export stopathlist=()


# print help
function print_help()
{
	echo -e "<<# # # - - - - - - - - - - - help - - - - - - - - - - - # # #"
	echo -e "01)-startall		./startall.sh -startall"
	echo -e "02)-start		./startall.sh -start exe	eg:./startall.sh -start /opt/smsp/niumeihua/smsp5.5/build/smsp_access/bin/smsp_access"
	echo -e "03)-stopall		./startall.sh -stopall"
	echo -e "04)-stop		./startall.sh -stop exe		eg:./startall.sh -stop /opt/smsp/niumeihua/smsp5.5/build/smsp_access/bin/smsp_access"
	echo -e "05)-restartall		./startall.sh -restartall"
	echo -e "06)-restart		./startall.sh -restart exe	eg:./startall.sh -restart /opt/smsp/niumeihua/smsp5.5/build/smsp_access/bin/smsp_access"
	echo -e "07)-psall		./startall.sh -psall"
	echo -e "08)-ps			./startall.sh -ps exe		eg:./startall.sh -ps smsp_access"
	echo -e "09)-pstart		./startall.sh -pstart"
	echo -e "10)-pstop		./startall.sh -pstop"
	echo -e "11)-help		./startall.sh -help"
	echo -e "<<# # # - - - - - - - - - - - help - - - - - - - - - - - # # #"
}

## get current starting files of the catalog
function get_current_start_catalog()
{
	for file in $scriptpath/*
	do
		if [ ! -d "$file" ]; then 
			echo "- [get_current_start_catalog:$LINENO] '$file' is not catalog!" 
			continue
		fi
		
		filename=${file##*/}
		for exefile in "${exefilelist[@]}"
		do
			if [ "$filename" != "$exefile" ]; then
				continue
			fi
			exepathlist[${#exepathlist[@]}]=$file
			#echo "+ [get_current_start_catalog:$LINENO] add ${#exepathlist[@]} '$file' to exepathlist" 
		done
	done
	echo "+ [get_current_start_catalog:$LINENO] totle is ${#exepathlist[@]}"
}

## print start_exe-file-path from the 'exepathlist'
function print_start_exepathlist()
{
	get_current_start_catalog
	echo "result ------------------------------------------------ 0"
	for exepath in "${exepathlist[@]}"
	do
		echo $exepath
	done
	echo "- [print_start_exepathlist:$LINENO] useable starting file is ${#exepathlist[@]}"
	echo "result ------------------------------------------------ 1"
}

# start a server
function start_one_svr()
{
	if [ ! $1 ] ; then
		echo "- [start_one_svr:$LINENO] starting exe-file param is not, add exe-file param please!" 
		print_help
		return
	fi
	execatalogpath=$1
	exe=${execatalogpath##*/}

	echo "$exe starting ..."
	if [ ! -d "$execatalogpath" ]; then 
		echo "- [start_one_svr:$LINENO] '$execatalogpath' is not exist!" 
		echo "- [start_one_svr:$LINENO] start the '$execatalogpath' is failed!" 
		return
	fi
	
	exefilepath=$execatalogpath/bin/$exe
	if [ ! -f "$exefilepath" ]; then 
		echo "- [start_one_svr:$LINENO] '$exefilepath' is not exist!" 
		echo "- [start_one_svr:$LINENO] start the '$exefilepath' is failed!" 
		return
	fi
	
	check_process $exefilepath
	if [ $? -eq 1 ]; then
		echo "- [start_one_svr:$LINENO] start the '$exefilepath' is failed!" 
		return
	fi
	
	nohup $exefilepath > $exe.log 2>&1 & 
	if [ $? -eq 1 ]; then
		echo "- [start_one_svr:$LINENO] start the '$exefilepath' is failed!" 
	else
		echo "+ [start_one_svr:$LINENO] start the '$exefilepath' is success!" 
	fi
	sleep 1
}

function start_all_svr()
{
	get_current_start_catalog
	if [ ${#exepathlist[@]} -eq 0 ]; then
		echo "- [start_all_svr:$LINENO] get the number of starting exe-file-path is zero!"
		return
	fi
	
	for exepath in "${exepathlist[@]}"
	do
		start_one_svr $exepath
	done
	
	ps_svr 
}

## get current stopping files of the catalog
function get_current_stop_catalog()
{
	get_current_start_catalog
	for exepath in "${exepathlist[@]}"
	do
		exe=${exepath##*/}
		exefilepath=$exepath/bin/$exe
		result=`ps -ef|grep "$exefilepath" | grep -v "grep" | awk '{print $8}'`
		if [ -z "$result" ]; then
			echo "- [get_current_stop_catalog] the process '$exefilepath' is not exit!"
			exefilepath=""
			continue
		fi
		stopathlist[${#stopathlist[@]}]=$exefilepath
		#echo "+ [get_current_stop_catalog:$LINENO] add ${#stopathlist[@]} '$exefilepath' to stopathlist"
		exefilepath=""
	done
	echo "+ [get_current_stop_catalog:$LINENO] totle is ${#stopathlist[@]}"
}

# print stop_exe-file-path from the 'exepathlist'
function print_stop_exepathlist()
{
	get_current_stop_catalog
	echo "result ------------------------------------------------ 0"
	for exepath in "${stopathlist[@]}"
	do
		echo $exepath
	done
	echo "+ [print_stop_exepathlist] useable stopping file is ${#stopathlist[@]}"
	echo "result ------------------------------------------------ 1"
}


## stop a server
function stop_one_svr()
{
	if [ ! $1 ] ; then
		echo "- [stop_one_svr:$LINENO] stoppig exe-file param is not, add exe-file param please!" 
		print_help
		return
	fi
	exefilepath=$1
	if [ ! -f "$exefilepath" ]; then 
		echo "- [stop_one_svr:$LINENO] '$exefilepath' is not exist!" 
		echo "- [stop_one_svr:$LINENO] stop the '$exefilepath' is failed!" 
		return
	fi
	
	killall $exefilepath
	if [ $? -eq 1 ]; then
		echo "- [stop_one_svr:$LINENO] stop the '$exefilepath' is failed!" 
	else
		echo "+ [stop_one_svr:$LINENO] stop the '$exefilepath' is success!" 
	fi
	sleep 1
}

## stop all server
function stop_all_svr()
{
	get_current_stop_catalog
	if [ ${#stopathlist[@]} -eq 0 ]; then
		echo "- [stop_all_svr] get the number of starting exe-file-path is zero!"
		return
	fi
	
	for exepath in "${stopathlist[@]}"
	do
		stop_one_svr $exepath
	done
	
	ps_svr 
}

## ps -ef|grep svr
function ps_svr()
{
	echo "result ------------------------------------------------ 0"
	if [ ! $1 ] ; then
		ps -ef|grep smsp_ | grep -v "grep"
		echo "result ------------------------------------------------ 1"
		return
	fi
	
	ps -ef|grep $1 | grep -v "grep"
	echo "result ------------------------------------------------ 1"
}

## check server
function check_process()
{
	exefilepath=$1
	var=`ps aux | grep -i "$1" | grep -v grep | awk '{print $2}'`
	OLD_IFS="$IFS" 
	IFS=" " 
	array=($var) 
	IFS="$OLD_IFS"
	
	if [ ${#array} -eq 0 ]; then
		echo "+ [check_process:$LINENO] the process of '$exefilepath' is not exist!" 
		return 0
	else
		echo "- [check_process:$LINENO] the process of '$exefilepath' is exist!" 
		return 1
	fi
	
}

## restart all server
function restart_all_svr()
{
	stop_all_svr && sleep 1 && start_all_svr
}

## restart server
function restart_svr()
{
	if [ ! $1 ] ; then
		echo "- [restart_svr:$LINENO] stoppig exe-file param is not, add exe-file param please!" 
		print_help
		return
	fi
	stop_svr $1 && sleep 1 && start_svr $1
}

function main()
{
	if [ ! $1 ] ; then
		start_all_svr
		return
	fi
	
	case $1 in
		-startall)
			start_all_svr
			;;
		-stopall)
			stop_all_svr
			;;
		-restartall)
			restart_all_svr
			;;
		-psall)
			ps_svr
			;;
		-ps)
			ps_svr $2
			;;
		-start)
			start_one_svr $2
			ps_svr
			;;
		-pstart)
			print_start_exepathlist
			;;
		-stop)
			stop_one_svr $2
			ps_svr
			;;
		-pstop)
			print_stop_exepathlist
			;;
		-restart)
			restart_svr $2
			;;
		-help)
			print_help
			;;
		*)
			print_help
			;;
	esac
	
	exit 0
	#get_current_start_catalog
	#print_exepathlist
	#start_one_svr $1
	#start_all_svr
	#stop_one_svr
	#get_current_stop_catalog
	#print_stop_exepathlist
	#stop_all_svr
}

main $@
