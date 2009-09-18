#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: analyzeDVFS.pl
 # DESCRIPTION: This takes in an entire dir full of log dirs (Output of pull.pl)
 # and generates a single file which summarizes it. 
 # -bench will print the per workload summary, -cpu will print the summary 
 # from a workload agnostic point of view. 
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Amithash Prasad                                              *
 #                                                                             *
 # This file is part of Seeker                                                 *
 #                                                                             *
 # Seeker is free software: you can redistribute it and/or modify it under the *
 # terms of the GNU General Public License as published by the Free Software   *
 # Foundation, either version 3 of the License, or (at your option) any later  *
 # version.                                                                    *
 #                                                                             *
 # This program is distributed in the hope that it will be useful, but WITHOUT *
 # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 # FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 # for more details.                                                           *
 #                                                                             *
 # You should have received a copy of the GNU General Public License along     *
 # with this program. If not, see <http://www.gnu.org/licenses/>.              *
 #*****************************************************************************/

#use warnings;
use Getopt::Long;
use File::Find;
use Cwd;
use Time::HiRes qw( gettimeofday tv_interval);

############################################################################## 
# GLOBALS 							             #
##############################################################################
my $currentdir = cwd();
my $org_path = cwd();
my $help;
my $targetdir;

my $cpu_power = 0;
my $bench_power = 0;
my $states = 5;

open LOG, "+>LOG" or die "No permissions to create LOG\n";

###############################################################################
# SUBROUTINES                                                                 #
###############################################################################
sub help {
    print ("profileDVFS.pl -targetdir (dir) \n");
    exit 1;
}

$PowerState[0] = 1100;
$PowerState[1] = 1400;
$PowerState[2] = 1700;
$PowerState[3] = 2000;
$PowerState[4] = 2200;

$PowerState2[0] = 1100;
$PowerState2[1] = 2200;

$PowerValue[0] = 64.6;
$PowerValue[1] = 75.6;
$PowerValue[2] = 88.8;
$PowerValue[3] = 108.2;
$PowerValue[4] = 115.0;

$PowerValue2[0] = 64.6;
$PowerValue2[1] = 115.0;

$ref_clockspeed = 2200;

sub gatherRunTime
{
	my $timef = shift;
	open IN, "cat $timef | gunzip |" or die "Could not open $timef\n";
	while(my $line = <IN>){
		chomp($line);
		my @tmp = split(/\s/,$line);
		my $bench = $tmp[0];
		my $rtime = $tmp[1];
		$benchRunningTime{$bench} = $rtime;
	}
	close(IN);
}

sub collectStats
{
    my $targetdir = $_[0];
    chdir($targetdir);
    
    if ($cpu_power) {
        gatherCPUresults($targetdir);
    }
    
    if ($bench_power) {
	gatherBenchmarkresults($targetdir);
	if($targetdir =~ /log_(.+)$/){
		my $exp = $1;
		my $tfile = "../time_$exp.txt.gz";
		if(! -e $tfile){
			if($exp =~/(.+)_([A-Za-z\-]+)/){
				my $ex1 = $1;
				my $ex2 = $2;
				$tfile = "../time_$1__$2.txt.gz";
			}
		}
		gatherRunTime($tfile);
	} else {
		print "$targetdir does not match\n";
	}
    }
    chdir($currentdir);
}

sub gatherCPUresults 
{
    my $experiment = $_[0];
    #print "$experiment\n";
    @cpu_files = <CPUST_?.gz>;
    foreach $cpu (@cpu_files) {
	readCPUfile($experiment, $cpu); 
    } 
}

sub gatherBenchmarkresults 
{
    my $experiment = $_[0];
    @sch_files = <*.sch.gz>;
    foreach $sch (@sch_files) {
	readBenchmarkfile($experiment, $sch);
    } 
    
}


sub readCPUfile
{
    my $experiment = $_[0];
    my $statsfile = $_[1];
    my $n_cpu;

    $ProjectFiles{"$experiment:$statsfile"} = 1;
    
    if ($statsfile =~ /CPUST_(\d+)/) {
	$n_cpu = $1;
    }
    die("Invalid CPU file\n") unless(defined($n_cpu));

    print LOG "$statsfile\n";
    
    open ST, "cat $statsfile | gunzip |" or die "cannot open $statsfile";
    my @lines = <ST>;
    foreach $line (@lines) {
	chomp($line);
	#print "Trying to read $line\n";
	if($line =~ m/(\d+)\s(\d+)/) {
	    $state = $1;
	    $duration = $2;
	    $Profile{"$experiment:$statsfile"}[$state] += $duration;
        }
    }
    close(ST);
}

sub readBenchmarkfile
{
    my $experiment = $_[0];
    my $statsfile = $_[1];
    my $bench;
    
    
    if ($statsfile =~ /(\S+).sch/) {
	$bench  = $1;
    }
    die("Invalid benchmark file\n") unless(defined($bench));
    $Benchmarks{"$bench"} = 1;
    
    
    #print "Working on $bench \n";
    
    $BenchInstData{$bench} = 0;
    $BenchCycleData{$bench} = 0;
    for ($i=0; $i < $states; $i++) {
	$BenchStateInstData{$bench}[$state] =0;
	$BenchStateCycleData{$bench}[$state] =0;
    }
    
    open ST, "cat $statsfile | gunzip |" or die "cannot open $statsfile";
    my @lines = <ST>;
    foreach $line (@lines) {
	chomp($line);
	#print "Trying to read $line\n";
        # 7715663045 7781310906 0 219632472 201791839 2 1.1250 4 4 4
	if($line =~ m/(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+\.\d+)\s(\d+)\s(\d+)\s(\d+)/) {
	    #tot_inst - total cumilative instructions. Useful for timeline plots.
	    #tot_refcy - Total cumilative reference cycles (Do not change with clock speed).
	    #interval - Mutation interval. 0 for fixed, but increments every mutation for the delta runs.
	    #insts - Total instructions in this sample.
	    #refcy - total reference cycles in this sample.
	    #cpu - cpu on which this sample executed on.
	    #reipc - real IPC of sample. reipc uses real cycles which change with clock speed.
	    #state - state on which this sample executed on.
	    #req_state - the state which this sample requested.
	    #giv_state - the state given instead to this sample.
	    
	    $tot_inst = $1;
	    $tot_refcy = $2;
	    $interval = $3;
	    $inst = $4;
	    $cycle = $5;
	    $cpu = $6;
	    $ipc = $7;
	    $state = $8;
	    $request = $9;
	    $given = $10; 
             
            if($cycle > (100 * (10 ** 9))){
		print STDERR "Too big sample, bad data\n";
		next;	
	    }
	    
	    $BenchInstData{$bench} += $inst;
	    $BenchCycleData{$bench} += $cycle;
#	print "What is cycle $cycle   $BenchCycleData{$bench}\n";
	    $BenchStateInstData{$bench}[$state] += $inst;
	    $BenchStateCycleData{$bench}[$state] += $cycle;
	    
	    # y = 15.768 * 10 ^(0.0007x)
	    #$power = 15.768 * (exp(.0007 * $PowerState[$state]));
	    
        }
	else {
	    print LOG ("$experiment : $statsfile : Not working yet\n");
	}
    }
    close(ST);
}

sub summarizeResults 
{
    my $experiment = $_[0];
    
    # y = 15.768 * 10 ^(0.0007x)
    #$power = 15.768 * (exp(.0007 * $PowerState[$state]));
    
    if ($cpu_power) {
	printf("CPU-Power $experiment ");
	$total_energy = 0; 
        $total_time = 0;
	foreach $proj (keys %ProjectFiles) {
	    printf(": ");
	    for ($i=0; $i < $states; $i++) {
		$time = $Profile{"$proj"}[$i] / 1000.0; # Time is in milli seconds. 
		$power = $PowerValue[$i] if($states == 5);
		$power = $PowerValue2[$i] if($states == 2);
		$energy = $time * $power; # Energy in joules.
		printf("%d ", $Profile{"$proj"}[$i]);
	        $total_energy = $total_energy + $energy; 
                $total_time = $total_time + $time;
	    }
	}
	if($total_time > 0.0){
 	    printf (" : Energy $total_energy : Average Power %3.3f\n",$total_energy / $total_time);
        } else {
           print LOG "Error! Bad data file";
        }
    }
    
    if ($bench_power) {
	foreach $bench (keys %Benchmarks) {
	    print "$experiment $bench ";
	    $total_inst = $BenchInstData{$bench};
	    $total_cycle = $BenchCycleData{$bench};
            $total_energy = 0;
	    
	    printf "$total_cycle $total_inst ";
	    my $error = ""; 
	    for ($i=0; $i < $states; $i++) {
		$inst = $BenchStateInstData{$bench}[$i];
		$cycle = $BenchStateCycleData{$bench}[$i];
		$power = $PowerValue[$i] if($states == 5);
                $power = $PowerValue2[$i] if($states == 2);
		$time = $cycle / ($ref_clockspeed * (10 ** 6));
		$energy = $time * $power;
		$total_energy += $energy;
		if($total_cycle == 0){
                        $error .= "Bad data file!\n";
			printf "%3.3f ", 0;
			next;
		}
		printf "%3.3f ", $cycle/$total_cycle;
	    }
	    print "$total_energy ";
	    if(not defined($benchRunningTime{$bench})){
                printf "%3.3f\n", 0;
		$error .= "running time for bench could not be collected\n";
	    } else {
		printf "%3.3f\n", $benchRunningTime{$bench};
            }
	  
            print LOG "$error" if($error ne "");
	}
    }
}

###############################################################################
## MAIN
###############################################################################
GetOptions ('target:s' => \$targetdir,
            'list:s' => \$list,
            'cpu' => \$cpu_power,
            'bench' => \$bench_power,
	    'states=i' => \$states,
	    'help' => \$help);

# Ask for help
if(defined $help){
    help();
}

if($states != 2 and $states != 5){
    print "--states can take a value of 2 or 5. Default: 5\n";
    exit;
}
print LOG "Total states are: $states\n";

if (defined $targetdir) {
    # Run the result gathering function
    collectStats($targetdir);
    summarizeResults($targetdir);
}


if (defined $list) {
    
    @dir_list  = <log*$list*>;
    foreach $dir (@dir_list) {
	
	print LOG "Working on $dir \n";
	collectStats($dir);
        summarizeResults($dir);
	%ProjectFiles = ();
        %Benchmarks = ();
        %BenchStateInstData = ();
        %BenchStateCycleData = ();
        %BenchCycleData = ();
        %benchRunningTime = ();

        %BenchInstData = ();
        %Profile = ();
    }
}


