#!/bin/bash

rm -r /root/group_4
rm -r /root/group_8
mkdir /root/group_4
mkdir /root/group_8

function mailme {
	cat /root/mother.err > /root/tmp_mail
	cat /root/mother.out >> /root/tmp_mail
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

for i in 0 1 2 3 4 ;
do
	for cpus in 4 ;
	do
		for j in ${BLS[@]}
		do
			${SEEKER_HOME}/AnalysisScripts/generate_runscript.pl --benchlist=${SEEKER_HOME}/AnalysisScripts/group_${cpus}/${j} -a 
			echo chmod +x ./run.sh
			insmod $SEEKER_HOME/Build/seeker_scheduler.ko disable_scheduling=1 static_layout=${i},${i},${i},${i},${i},${i},${i},${i} allowed_cpus=${cpus} >> /root/mother.out 2>> /root/mother.err
			$SEEKER_HOME/Scripts/debugd /root/group_${cpus}/log_ >> /root/mother.out 2>> /root/mother.err
			sleep 10
			rm ./LOG
			$BENCH_ROOT/run.sh
			sleep 10
			$SEEKER_HOME/Scripts/send.pl -t >> /root/mother.out 2>> /root/mother.err
			mv ./LOG /root/group_${cpus}/TIME_${j}_${i} >> /root/mother.out 2>> /root/mother.err
			$SEEKER_HOME/Scripts/decodelog < /root/group_${cpus}/log_0 > /root/group_${cpus}/log_${j}_${i}.txt
			rm /root/group_${cpus}/log_0 >> /root/mother.out 2>> /root/mother.err
			rmmod seeker_scheduler >> /root/mother.out 2>> /root/mother.err
			sleep 10
			mailme "state = ${i}, groups of ${cpus} benchlist=${j}"
			rm /root/mother.err
			rm /root/mother.out
		done
	done
done

