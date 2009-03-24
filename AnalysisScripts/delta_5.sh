#!/bin/bash

GROUP4=/home/prasadae/delta5_4
GROUP8=/home/prasadae/delta5_8

OUT=/home/prasadae/delta5.out
ERR=/home/prasadae/delta5.err

BLS=( HighIPC0 Low-HighIPC0 LowIPC1 PhaseHigh-PhaseLow PhaseLow-High PhaseLow-Low )
BLS_NAME=( High Low-High Low PHigh-PLow PLow-High PLow-Low )
BLS_LEN=${#BLS[@]}

CPUS=( 4 );
CPUS_LEN=${#CPUS[@]}

DELTA4=( 1 2 4 8 )
DELTA8=( 1 4 8 16 32 )
DELTA4_LEN=${#DELTA4[@]}
DELTA8_LEN=${#DELTA8[@]}

INTERVAL=( 125 250 500 )
INTERVAL_LEN=${#INTERVAL[@]}

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
$SEEKER_HOME/seeker_cpufreq.pl start

cd $BENCH_ROOT

# For each cpus (4,8)
for (( i=0;i<$CPUS_LEN;i++ )); do
	echo ${CPUS[$i]}
	DELTA=();
	DELTA_LEN=0;
	if [ ${CPUS[$i]} = 4 ]; then
		DELTA=( ${DELTA4[@]} )
		DELTA_LEN=$DELTA4_LEN
	else
		DELTA=( ${DELTA8[@]} )
		DELTA_LEN=$DELTA8_LEN
	fi
	GROUP="";
	if [ ${CPUS[$i]} = 4 ]; then
		GROUP=$GROUP4
	else
		GROUP=$GROUP8
	fi

	# For each interval. 
	for (( l=0;l<$INTERVAL_LEN;l++ )); do

		# For each delta
		for (( j=0;j<$DELTA_LEN;j++ )); do

			echo "Interval=${INTERVAL[$l]}, Delta=${DELTA[$j]}"

			for (( k=0;k<$BLS_LEN;k++ )); do

				TIME_NAME=$GROUP/time_${INTERVAL[$l]}_${DELTA[$j]}_${BLS_NAME[$k]}.txt
				RAW_NAME=$GROUP/raw_${INTERVAL[$l]}_${DELTA[$j]}_${BLS_NAME[$k]}.txt
				LOG_NAME=$GROUP/log_${INTERVAL[$l]}_${DELTA[$j]}_${BLS_NAME[$k]}
				BENCHLIST=$SEEKER_HOME/AnalysisScripts/group_${CPUS[$i]}/${BLS[$k]}

				# Allow restart.
				if [ -e $TIME_NAME ]; then
					echo "Found Experiment files, continuing"
					continue;
				fi

				insmod $SEEKER_HOME/Build/seeker_scheduler.ko allowed_cpus=${CPUS[$i]} change_interval=${INTERVAL[$l]} delta=${DELTA[$j]} >> $OUT 2>> $ERR
	
				${SEEKER_HOME}/AnalysisScripts/generate_runscript.pl --benchlist=${BENCHLIST} -a 
				chmod +x ./run.sh

				$SEEKER_HOME/Scripts/debugd $GROUP/log_ >> $OUT 2>> $ERR
				sleep 10
				rm ./LOG

				$BENCH_ROOT/run.sh >> $OUT 2>> $ERR
				sleep 10
				$SEEKER_HOME/Scripts/send.pl -t >> $OUT 2>> $ERR
				sleep 60
				rmmod seeker_scheduler
	
				$SEEKER_HOME/AnalysisScripts/ex_time.pl ./LOG ${TIME_NAME} >> $OUT 2>> $ERR
	
				$SEEKER_HOME/Scripts/decodelog < $GROUP/log_0 > $RAW_NAME
	
				$SEEKER_HOME/Scripts/pull.pl --input $RAW_NAME --output $LOG_NAME --benchlist $BENCHLIST --what=all >> $OUT 2>> $ERR
	
				rm $GROUP/log_0 >> $OUT 2>> $ERR

				mailme "Cpus=${CPUS[$i]}, Interval=${INTERVAL[$l]}, Delta=${DELTA[$j]} bench group=${BLS_NAME[$k]}, just got over"
				rm $OUT
				rm $ERR
			done
		done
	done
done
mailme "All done, come and get it!"

$SEEKER_HOME/seeker_cpufreq.pl stop
rmmod seeker_cpufreq
rmmod pmu



