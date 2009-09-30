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

GROUP4=/home/prasadae/group_4
GROUP8=/home/prasadae/group_8

OUT=/home/prasadae/fixed.out
ERR=/home/prasadae/fixed.err

BLS=( HighIPC0 Low-HighIPC0 LowIPC1 PhaseHigh-PhaseLow PhaseLow-High PhaseLow-Low )
BLS_NAME=( High Low-High Low PHigh-PLow PLow-High PLow-Low )
BLS_LEN=${#BLS[@]}

CPUS=( 4 );
CPUS_LEN=${#CPUS[@]}

LAYOUT4=( '0,0,0,0' '4,4,4,4' '0,0,4,4' '0,4,4,4' '0,0,0,4' )
LAYOUT4_LEN=${#LAYOUT4[@]}
LAYOUT4_NAME=( '4_0' '0_4' '2_2' '1_3' '3_1' )

LAYOUT8=( '0,0,0,0,4,4,4,4' '0,0,4,4,4,4,4,4' '0,0,0,0,0,0,4,4' '0,0,0,0,0,0,0,0' '4,4,4,4,4,4,4,4' )
LAYOUT8_LEN=${#LAYOUT8[@]}
LAYOUT8_NAME=( '4_4' '2_6' '6_2' '8_0' '0_8' )

if [ ! -d $GROUP4 ]; then
	mkdir $GROUP4
fi

if [ ! -d $GROUP8 ]; then
	mkdir $GROUP8
fi

function mailme {
	cat $ERR > /home/prasadae/tmp_mail
	cat $OUT >> /home/prasadae/tmp_mail
	echo "Regards, AMDBox" >> /home/prasadae/tmp_mail
	cat /home/prasadae/tmp_mail | mail -s "$1" amithash@gmail.com
}

if [ `lsmod | grep powernow_k8 | wc -l` -eq 0 ]; then
	echo 'no powernow, inserting.'
	modprobe powernow_k8
fi

insmod $SEEKER_HOME/Build/pmu.ko
insmod $SEEKER_HOME/Build/seeker_cpufreq.ko
$SEEKER_HOME/Scripts/seeker_cpufreq.pl start

# For each cpus (4,8)
for (( i=0;i<$CPUS_LEN;i++ )); do
	echo ${CPUS[$i]}
	LAYOUT=();
	LAYOUT_NAME=();
	LAYOUT_LEN=0;
	if [ ${CPUS[$i]} = 4 ]; then
		LAYOUT=( ${LAYOUT4[@]} )
		LAYOUT_LEN=$LAYOUT4_LEN
		LAYOUT_NAME=( ${LAYOUT4_NAME[@]} )
	else
		LAYOUT=( ${LAYOUT8[@]} )
		LAYOUT_LEN=$LAYOUT8_LEN
		LAYOUT_NAME=( ${LAYOUT8_NAME[@]} )
	fi
	GROUP="";
	if [ ${CPUS[$i]} = 4 ]; then
		GROUP=$GROUP4
	else
		GROUP=$GROUP8
	fi

	# For each layout possible. 
	for (( j=0;j<$LAYOUT_LEN;j++ )); do
		echo "layout=${LAYOUT[$j]}"
		echo "layout Name=${LAYOUT_NAME[$j]}"
		insmod $SEEKER_HOME/Build/seeker_scheduler.ko mutation_method=4 init_layout=${LAYOUT[$j]} allowed_cpus=${CPUS[$i]} >> $OUT 2>> $ERR
		for (( k=0;k<$BLS_LEN;k++ )); do

			# Allow restart.
			if [ -e $GROUP/time_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}.txt ]; then
				echo "Found Experiment files, continuing"
				continue;
			fi

			BENCHLIST=$SEEKER_HOME/MiscScripts/benchlists/group_${CPUS[$i]}/${BLS[$k]}

			${SEEKER_HOME}/MiscScripts/generate_runscript.pl --benchlist=${SEEKER_HOME}/MiscScripts/group_${CPUS[$i]}/${BLS[$k]} -a 
			chmod +x ./run.sh
			$SEEKER_HOME/Scripts/seekerlogd $GROUP/log_ >> $OUT 2>> $ERR
			sleep 10
			rm ./LOG
			$BENCH_ROOT/run.sh >> $OUT 2>> $ERR
			sleep 10
			$SEEKER_HOME/Scripts/send.pl -t >> $OUT 2>> $ERR

			$SEEKER_HOME/MiscScripts/ex_time.pl ./LOG $GROUP/time_${LAYOUT_NAME[$j]}__${BLS_NAME[$k]}.txt >> $OUT 2>> $ERR

			$SEEKER_HOME/Scripts/decodelog < $GROUP/log_0 > $GROUP/raw_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}.txt 

			$SEEKER_HOME/Scripts/pull.pl --input $GROUP/raw_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}.txt              \
				                    --output $GROUP/log_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}                  \
						    --benchlist $BENCHLIST						    \
						    --what=all >> $OUT 2>> $ERR

			rm $GROUP/log_0 >> $OUT 2>> $ERR
			mailme "Cpus=${CPUS[$i]}, Layout=${LAYOUT_NAME[$j]}, bench group=${BLS_NAME[$k]}, just got over"
			rm $OUT
			rm $ERR
			sleep 60
		done
		rmmod seeker_scheduler
	done
done
mailme "All done, come and get it!"

$SEEKER_HOME/Scripts/seeker_cpufreq.pl stop
rmmod seeker_cpufreq
rmmod pmu



