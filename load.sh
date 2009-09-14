#!/bin/bash
#*************************************************************************
# Copyright 2009 Amithash Prasad                                         *
#                                                                        *
# This file is part of Seeker                                            *
#                                                                        *
# Seeker is free software: you can redistribute it and/or modify         *
# it under the terms of the GNU General Public License as published by   *
# the Free Software Foundation, either version 3 of the License, or      *
# (at your option) any later version.                                    *
#                                                                        *
# This program is distributed in the hope that it will be useful,        *
# but WITHOUT ANY WARRANTY; without even the implied warranty of         *
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
# GNU General Public License for more details.                           *
#                                                                        *
# You should have received a copy of the GNU General Public License      *
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
#*************************************************************************


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
   -b      Base state Default: 0
   -c	   Allowed number of cpus Default: 4
   -d      Delta value Default: 1
   -l      Comma seperated initialization layout Default: 0,0,0,0
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

if [ -z $SEEKER_HOME ]; then
  echo "SEEKER_HOME Environment variable is not set. Please set it and build seeker first."
fi

if [ -f $SEEKER_HOME/Build/pmu.ko && -f $SEEKER_HOME/Build/seeker_cpufreq.ko && -f $SEEKER_HOME/Build/seeker_scheduler.ko && -f $SEEKER_HOME/Build/seekerlogd ]; then
  echo "Gathering module options"
else
  echo "Please build your modules by performing the following:"
  echo "cd ${SEEKER_HOME}"
  echo "make"
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
	modprobe $DRIVER
fi

if [ `lsmod | grep seeker_scheduler | wc -l` -ne 0 ]; then
  echo "seeker_scheduler seems to be loaded. Please unload it by runing unload.sh"
  exit;
fi
if [ `lsmod | grep seeker_cpufreq | wc -l` -ne 0 ]; then
  echo "seeker_cpufreq seems to be loaded. Please unload it by runing unload.sh"
  exit;
fi
if [ `lsmod | grep pmu | wc -l` -ne 0 ]; then
  echo "pmu seems to be loaded. Please unload it by runing unload.sh"
  exit;
fi


insmod $SEEKER_HOME/Build/pmu.ko
insmod $SEEKER_HOME/Build/seeker_cpufreq.ko
$SEEKER_HOME/seeker_cpufreq.pl start
insmod $SEEKER_HOME/Build/seeker_scheduler.ko change_interval=$MUTATION_INTERVAL mutation_method=$MUTATION_METHOD base_state=$BASE_STATE allowed_cpus=$NR_CPUS scheduling_method=$SCHEDULING_METHOD init_layout=$LAYOUT

