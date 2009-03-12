#!/bin/bash

rm -r /root/group_4
rm -r /root/group_8
mkdir /root/group_4
mkdir /root/group_8

function mailme {
	cat /root/fixed.err > /root/tmp_mail
	cat /root/fixed.out >> /root/tmp_mail
	echo "Regards, AMDBox" >> /root/tmp_mail
	cat /root/tmp_mail | mail -s "$1" amithash@gmail.com
}

A=`lsmod | grep powernow_k8 | wc -l`
if [ $A -eq 0 ]
then
	echo 'no powernow, inserting.'
	modprobe powernow_k8
fi
insmod $SEEKER_HOME/Build/pmu.ko
insmod $SEEKER_HOME/Build/seeker_cpufreq.ko
$SEEKER_HOME/seeker_cpufreq.pl start

BLS=( HighIPC0 HighIPC1 Low-HighIPC0 Low-HighIPC1 LowIPC0 LowIPC1 PhaseHigh-High PhaseHigh-Low PhaseHigh-PhaseLow PhaseLow-High PhaseLow-Low )
BLS_NAME=( High0 High1 Low-High0 Low-High1 Low0 Low1 PHigh-High PHigh-Low PHigh-PLow PLow-High PLow-Low )
BLS_LEN=${#BLS[@]}

CPUS=( 4 8 );
CPUS_LEN=${#CPUS[@]}

LAYOUT4=( '0,0,4,4' '0,4,4,4' '0,0,0,4' )
LAYOUT4_LEN=${#LAYOUT4[@]}
LAYOUT4_NAME=( '2_2' '1_3' '3_1' )

LAYOUT8=( '0,0,0,0,4,4,4,4' '0,0,0,4,4,4,4,4' '0,0,0,0,0,4,4,4' '0,0,4,4,4,4,4,4' '0,0,0,0,0,0,4,4' '0,4,4,4,4,4,4,4' '0,0,0,0,0,0,0,4' )
LAYOUT8_LEN=${#LAYOUT8[@]}
LAYOUT8_NAME=( '4_4' '3_5' '5_3' '2_6' '6_2' '1_7' '7_1' )

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

	# For each layout possible. 
	for (( j=0;j<$LAYOUT_LEN;j++ )); do
		echo "layout=${LAYOUT[$j]}"
		echo "layout Name=${LAYOUT_NAME[$j]}"
		insmod $SEEKER_HOME/Build/seeker_scheduler.ko static_layout=${LAYOUT[$j]} allowed_cpus=${CPUS[$i]} >> /root/fixed.out 2>> /root/fixed.err
		for (( k=0;k<$BLS_LEN;k++ )); do
			${SEEKER_HOME}/AnalysisScripts/generate_runscript.pl --benchlist=${SEEKER_HOME}/AnalysisScripts/group_${CPUS[$i]}/${BLS[$k]} -a 
			chmod +x ./run.sh
			$SEEKER_HOME/Scripts/debugd /root/group_${CPUS[$i]}/log_ >> /root/fixed.out 2>> /root/fixed.err
			sleep 10
			rm ./LOG
			$BENCH_ROOT/run.sh >> /root/fixed.out 2>> /root/fixed.err
			sleep 10
			$SEEKER_HOME/Scripts/send.pl -t >> /root/fixed.out 2>> /root/fixed.err
			$SEEKER_HOME/AnalysisScripts/ex_time.pl ./LOG /root/group_${CPUS[$i]}/time_${LAYOUT_NAME[$j]}__${BLS_NAME[$k]}.txt >> /root/fixed.out 2>> /root/fixed.err
			$SEEKER_HOME/Scripts/decodelog < /root/group_${CPUS[$i]}/log_0 > /root/group_${CPUS[$i]}/raw_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}.txt 
			$SEEKER_HOME/Scripts/pull.pl --input /root/group_${CPUS[$i]}/raw_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]}.txt --output /root/group_${CPUS[$i]}/log_${LAYOUT_NAME[$j]}_${BLS_NAME[$k]} --benchlist ${SEEKER_HOME}/AnalysisScripts/group_${CPUS[$i]}/${BLS[$k]} --what=all >> /root/fixed.out 2>> /root/fixed.err
			rm /root/group_${CPUS[$i]}/log_0 >> /root/fixed.out 2>> /root/fixed.err
			mailme "Cpus=${CPUS[$i]}, Layout=${LAYOUT_NAME[$j]}, bench group=${BLS_NAME[$k]}, just got over"
			rm /root/fixed.out
			rm /root/fixed.err
		done
		rmmod seeker_scheduler
	done
done
mailme "All done, come and get it!"

$SEEKER_HOME/seeker_cpufreq.pl stop
rmmod seeker_cpufreq
rmmod pmu



