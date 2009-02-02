#!/bin/sh

if [ $# != 3 ]; then
	echo "Usage: $0 <InDir> <OutDir> <TRIALS>"
	exit
fi

indir=$1
outdir=$2
trials=$3

# Constants
export TYPE_ARRAY="default alt1 alt2"
export DELTA_ARRAY="1 2 4 8 16"
export STATE_ARRAY="0 1"

if [ ! -d $indir ]; then
	echo "\"$indir\" does not exist or is not a directory"
	exit
fi

if [ ! -d $outdir ]; then
	echo "\"$outdir\" does not exist, creating it"
	mkdir $outdir
fi

tr=0
while [ $tr -lt $trials ];
do
	let t=$tr+1
	outd_spec="$outdir/trial$t"
	mkdir $outd_spec
	
	echo ""
	echo ""
	echo "Trial ${t}"
	echo ""
	echo ""

	for i in ${TYPE_ARRAY};
	do
		mkdir $outd_spec/$i
		for j in ${DELTA_ARRAY};
		do
			mkdir $outd_spec/${i}/delta_${j}
			./convert.pl $indir/${i}/delta_spec_trial_${t}_${j}.csv $outd_spec/${i}/delta_${j}
			./pprofile.pl $indir/${i}/power_spec_${t}_${j} $outd_spec/${i}/delta_${j}/PPROFILE
		done
	done

	for state in ${STATE_ARRAY};
	do
		./pprofile.pl $indir/base/power_spec_dry_state_${state}_trial_${t} $outdir/trial${t}/power_${state}
	done
	let tr=$tr+1
done

