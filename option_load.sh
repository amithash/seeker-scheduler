#!/bin/bash

#!/bin/bash
# Argument = -t test -r server -p password -v

DRIVER="powernow_k8"

usage()
{
cat << EOF
usage: $0 options

This script loads seeker-scheduler with the given options.

OPTIONS:
   -h      Show this message
   -s      Scheduling method (ladder,select,aladder,disable) Default: ladder
   -m      Mutation method (mdyn,gd,od,cv,disable) Default: gd
   -i	   Mutation interval in milli-seconds Default: 1000
   -b      Base state 
   -c	   Allowed number of cpus
   -d      Delta value
   -l      Comma seperated initialization layout
EOF
}
SCHEDULING_METHOD="ladder"
MUTATION_METHOD="gd"
declare -i MUTATION_INTERVAL=1000
declare -i BASE_STATE=0
declare -i NR_CPUS=4
declare -i DELTA=1
LAYOUT="0,0,0,0"

while getopts “hs:m:i:b:c:d:l:” OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         s)
             SCHEDULING_METHOD=$OPTARG
             ;;
         m)
             MUTATION_METHOD=$OPTARG
             ;;
         i)
             MUTATION_INTERVAL=$OPTARG
             ;;
         b)
             BASE_STATE=$OPTARG
             ;;
         c)
             NR_CPUS=$OPTARG
             ;;
         d)
             DELTA=$OPTARG
             ;;
         l)
             LAYOUT=$OPTARG
             ;;
         ?)
             usage
             exit
             ;;
     esac
done

case $SCHEDULING_METHOD in
	ladder)
		SCHEDULING_METHOD=0
		;;
	select)
		SCHEDULING_METHOD=1
		;;
	aladder)
		SCHEDULING_METHOD=2
		;;
	disable)
		SCHEDULING_METHOD=4
		;;
	?)
		usage
		exit;
		;;
esac

case $MUTATION_METHOD in
	gd)
		MUTATION_METHOD=0
		;;
	mdyn)
		MUTATION_METHOD=1
		;;
	od)
		MUTATION_METHOD=2
		;;
	cv)
		MUTATION_METHOD=3
		;;
	disable)
		MUTATION_METHOD=4
		;;
	?)
		usage
		exit;
		;;
esac

if [ ! $MUTATION_INTERVAL -gt 0 ]; then
	echo "Mutation interval needs to be an integer greater than 0"
	usage
	exit;
fi

if [ ! $BASE_STATE -ge 0 ]; then
	echo "Base state needs to be an integer greater than or equal to 0"
	usage
	exit;
fi

if [ ! $NR_CPUS -gt 0 ]; then
	echo "allowed cpus needs to be an integer greater than 0"
	usage
	exit;
fi
if [ ! $DELTA -gt 0 ]; then
	echo "allowed cpus needs to be an integer greater than 0"
	usage
	exit;
fi



echo "SCHEDULING_METHOD=$SCHEDULING_METHOD"
echo "MUTATION_METHOD=$MUTATION_METHOD"
echo "MUTATION_INTERVAL=$MUTATION_INTERVAL"
echo "BASE_STATE=$BASE_STATE"
echo "NR_CPUS=$NR_CPUS"
echo "DELTA=$DELTA"
echo "LAYOUT=$LAYOUT"

if [ `lsmod | grep $DRIVER | wc -l` -eq 0 ]; then
	echo 'no powernow, inserting.'
	echo modprobe $DRIVER
fi

echo "insmod $SEEKER_HOME/Build/pmu.ko"
echo "insmod $SEEKER_HOME/Build/seeker_cpufreq.ko"
echo "$SEEKER_HOME/seeker_cpufreq.pl start"
echo "insmod $SEEKER_HOME/Build/seeker_scheduler.ko change_interval=$MUTATION_INTERVAL mutation_method=$MUTATION_METHOD base_state=$BASE_STATE allowed_cpus=$NR_CPUS scheduling_method=$SCHEDULING_METHOD init_layout=$LAYOUT"

