#!/usr/bin/perl
#*************************************************************************
# Copyright 2009 Dan Connors                                             *
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
my $file;

###############################################################################
# SUBROUTINES                                                                 #
###############################################################################
sub help {
    print ("profileDVFS.pl -targetdir (dir) \n");
   exit 1;
}

$PowerState[0] = 1800;
$PowerState[1] = 2000;
$PowerState[2] = 2200;
$PowerState[3] = 2400;
$PowerState[4] = 2600;

my $stats = 0;
my $max = 0;


sub max {
    my($max) = shift(@_);

    foreach $temp (@_) {
        $max = $temp if $temp > $max;
    }
    return($max);
}

sub min {
    my($min) = shift(@_);

    foreach $temp (@_) {
	 #print "checking $temp  min: $min\n";
        $min  = $temp if $temp < $min;
    }
    return($min);
}

sub avg {
   $count = 0; 
   $sum = 0;
    foreach $temp (@_) {
        $sum = $temp + $sum;
	$count++;
    }
    return($sum/$count);
}

sub readCPUSummaryFile
{
    my $bench;
    my $statsfile = $_[0];
    
    open ST, "< $statsfile" or die "cannot open $statsfile";
    my @lines = <ST>;
    foreach $line (@lines) {
	chomp($line);
	#print "$line\n";
	#CPU-Power log_125_2_PLow-Low : 1410162 15259 6065 2762 151651 : 746335 377327 366804 11155 83975 : 1327107 246792 5198 1299 5503 : 636648 159183 344507 222799 222762  : Power 398530105.951732
	if($line =~ m/CPU-Power\s(\S+)\s:\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s:\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s:\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s:\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s+:\sPower\s(\d+.\d+)/) {

	$experiment = $1;
       
        $cpu0_0 = $2;
        $cpu0_1 = $3;
        $cpu0_2 = $4;
        $cpu0_3 = $5;
        $cpu0_4 = $6;
        $cpu0_total = $cpu0_0 + $cpu0_1 + $cpu0_2 + $cpu0_3;

        $cpu1_0 = $7;
        $cpu1_1 = $8;
        $cpu1_2 = $9;
        $cpu1_3 = $10;
        $cpu1_4 = $11;
        $cpu1_total = $cpu1_0 + $cpu1_1 + $cpu1_2 + $cpu1_3;

        $cpu2_0 = $12;
        $cpu2_1 = $13;
        $cpu2_2 = $14;
        $cpu2_3 = $15;
        $cpu2_4 = $16;
        $cpu2_total = $cpu2_0 + $cpu2_1 + $cpu2_2 + $cpu2_3;

        $cpu3_0 = $17;
        $cpu3_1 = $18;
        $cpu3_2 = $19;
        $cpu3_3 = $20;
        $cpu3_4 = $21;
        $cpu3_total = $cpu3_0 + $cpu3_1 + $cpu3_2 + $cpu3_3;

	$CPUExpPower{$experiment} = $22;

	# example: log_500_8_Low-High
	if ($experiment =~ m/log_(\d+)_(\d+)_(\S+)/) {
		$mutation = $1;
		$delta = $2;
		$workload = $3;

		$WorkloadExp{"$workload"} = 1;
		$DeltaExp{"$delta"} = 1;
		$MutationExp{"$mutation"} = 1;

		$index = "$workload:$delta:$mutation";

		$max_cycles = max($cpu0_total, $cpu1_total, $cpu2_total, $cpu3_total);
		#print "Max: $max_cycles $cpu0_total, $cpu1_total, $cpu2_total, $cpu3_total \n";

		# register the max cycles the CPU cores ran
                $CPUExp{$index} = $max_cycles;

                $CPUExpState[0][0]{$index} = $cpu0_0;
                $CPUExpState[0][1]{$index} = $cpu0_1;
                $CPUExpState[0][2]{$index} = $cpu0_2;
                $CPUExpState[0][3]{$index} = $cpu0_3;
                $CPUExpState[0][4]{$index} = $cpu0_4;

                $CPUExpState[1][0]{$index} = $cpu1_0;
                $CPUExpState[1][1]{$index} = $cpu1_1;
                $CPUExpState[1][2]{$index} = $cpu1_2;
                $CPUExpState[1][3]{$index} = $cpu1_3;
                $CPUExpState[1][4]{$index} = $cpu1_4;

                $CPUExpState[2][0]{$index} = $cpu2_0;
                $CPUExpState[2][1]{$index} = $cpu2_1;
                $CPUExpState[2][2]{$index} = $cpu2_2;
                $CPUExpState[2][3]{$index} = $cpu2_3;
                $CPUExpState[2][4]{$index} = $cpu2_4;

                $CPUExpState[3][0]{$index} = $cpu3_0;
                $CPUExpState[3][1]{$index} = $cpu3_1;
                $CPUExpState[3][2]{$index} = $cpu3_2;
                $CPUExpState[3][3]{$index} = $cpu3_3;
                $CPUExpState[3][4]{$index} = $cpu3_4;


		#print "here: $line\n";
		#print "got the right answer: $CPUExpState[0][0]{$index} and $CPUExpState[0][4]{$index} \n";
	}
	else {
	   print "error \n";
	   exit(-1);
	}

        # put things together
	#print "MDW : $mutation $delta $workload\n";

        }
    }
    close(ST);
}


sub readBenchSummaryFile
{
    my $statsfile = $_[0];

    open ST, "< $statsfile" or die "cannot open $statsfile";
    my @lines = <ST>;
    foreach $line (@lines) {
        chomp($line);
       if($line =~ m/(\S+)\s:\s+(\S+)\s:\s+(\d+)\s(\d+.\d+)\s(\d+.\d+)\s(\d+.\d+)\s(\d+.\d+)/) {
             #print "$1 $2 $3\n";
	     $experiment = $1;
             $bench = $2;
             $cycles = $3;
	     $state0 = $4;
	     $state1 = $5;
	     $state2 = $6;
	     $state3 = $7;
	     $state4 = $8;

             $BenchmarkData{$bench}[$BenchmarkIndex{$bench}] = $cycles;
             $BenchmarkIndex{$bench} = $BenchmarkIndex{$bench} +1;
             $Benchmarks{$bench} = 1;

	     if ($experiment =~ m/log_(\d+)_(\d+)_(\S+)/) {
		  $mutation = $1;
		  $delta = $2;
		  $workload = $3;

	  	  $WorkloadExp{"$workload"} = 1;
		  $DeltaExp{"$delta"} = 1;
		  $MutationExp{"$mutation"} = 1;

		   $index = "$workload:$delta:$mutation:$bench";
		   $BenchmarkExp{$index} = $cycles;
		   $BenchmarkExpState{$index}[0] = $state0;
		   $BenchmarkExpState{$index}[1] = $state1;
		   $BenchmarkExpState{$index}[2] = $state2;
		   $BenchmarkExpState{$index}[3] = $state3;
		   $BenchmarkExpState{$index}[4] = $state4;

		   #print $BenchmarkExpState[0]{"$workload:$bench:$delta:$mutation"}, " ";
		   #print $BenchmarkExpState[1]{"$workload:$bench:$delta:$mutation"}, " ";
		   #print $BenchmarkExpState[2]{"$workload:$bench:$delta:$mutation"}, " ";
  		   #print "\n";
  	     }
	     else {
		print "Haven't been coded yet\n";
		exit(-1);
	     }
        }
    }
    close(ST);


      foreach $work (keys %WorkloadExp) {
        foreach $delta (keys %DeltaExp) {
          foreach $mut (keys %MutationExp) {

             my @array = ();
             foreach $bench (keys %Benchmarks) {
             $index = "$work:$delta:$mut:$bench";

                next if (not (defined $BenchmarkExp{$index}));
                #print "$index ->  $BenchmarkExp{$index} \n";

                $value = $BenchmarkExp{$index};
                push(@array, $value);
            }
            $max_value = max(@array);
            $min_value = min(@array);
            $index = "$work:$delta:$mut";
	    $BenchmarkWorkloadReponse{$index} = $min_value;
	    $BenchmarkWorkloadTime{$index} = $max_value;
            #printf "what is %3.2f \n", $BenchmarkWorkloadTime{$index};
        }
      }
   }
}

sub readStaticBenchSummaryFile
{
    my $statsfile = $_[0];

    open ST, "< $statsfile" or die "cannot open $statsfile";
    my @lines = <ST>;
    foreach $line (@lines) {
        chomp($line);
       if($line =~ m/(\S+)\s:\s+(\S+)\s:\s+(\d+)\s(\d+.\d+)\s(\d+.\d+)\s(\d+.\d+)\s(\d+.\d+)/) {
             #print "$1 $2 $3\n";
	     $experiment = $1;
             $bench = $2;
             $cycles = $3;
	     $state0 = $4;
	     $state1 = $5;
	     $state2 = $6;
	     $state3 = $7;
	     $state4 = $8;

             $BenchmarkData{$bench}[$BenchmarkIndex{$bench}] = $cycles;
             $BenchmarkIndex{$bench} = $BenchmarkIndex{$bench} +1;
             $Benchmarks{$bench} = 1;

	     if ($experiment =~ m/log_(\d+)_(\d+)_(\S+)/) {
		  $static = "$1" . ":" . "$2";
		  $workload = $3;

	  	  $WorkloadExp{"$workload"} = 1;
		  $StaticExp{$static} = 1;

		   $index = "$workload:$static:$bench";
		   $StaticBenchmarkExp{$index} = $cycles;
		   $StaticBenchmarkExpState{$index}[0] = $state0;
		   $StaticBenchmarkExpState{$index}[1] = $state1;
		   $StaticBenchmarkExpState{$index}[2] = $state2;
		   $StaticBenchmarkExpState{$index}[3] = $state3;
		   $StaticBenchmarkExpState{$index}[4] = $state4;

		   #print $BenchmarkExpState[0]{"$workload:$bench:$delta:$mutation"}, " ";
		   #print $BenchmarkExpState[1]{"$workload:$bench:$delta:$mutation"}, " ";
		   #print $BenchmarkExpState[2]{"$workload:$bench:$delta:$mutation"}, " ";
  		   #print "\n";
  	     }
	     else {
		print "Haven't been coded yet\n";
		exit(-1);
	     }
        }
    }
    close(ST);
}


sub printBenchSummary
{
    foreach $bench (keys %Benchmarks) {
	$min = $BenchmarkData{$bench}[0];
	for ($i=0; $i < $BenchmarkIndex{$bench}; $i++) {
	    if ($BenchmarkData{$bench}[$i] < $min) {
		$min = $BenchmarkData{$bench}[$i];
	    }
	}   
	$max = $BenchmarkData{$bench}[0];
	for ($i=0; $i < $BenchmarkIndex{$bench}; $i++) {
	    if ($BenchmarkData{$bench}[$i] > $max) {
		$max = $BenchmarkData{$bench}[$i];
	    }
	}   
	$sum = 0;
	for ($i=0; $i < $BenchmarkIndex{$bench}; $i++) {
            $sum  += $BenchmarkData{$bench}[$i];
	}   
	
	if ($print_stats) {
	    printf " %10s :", $bench;
	    for ($i=0; $i < $BenchmarkIndex{$bench}; $i++) {
		print " "; printf " %3.2f ", $BenchmarkData{$bench}[$i]/$min ;
	    }   
	    print "\n";
	}
	
	if ($print_max) {
	    printf " %10s :", $bench;
	    print " "; 
	    printf " %3.2f ", $max/$min ;
	    printf " %3.2f ", $sum/($min * $BenchmarkIndex{$bench}) ;
	    print "\n";
	}
    }
}

sub printCPUSummary
{
    my $statsfile = $_[0];

}

sub summarizeBestDynamicBench
{
  print "Best benchmark runs using dynamic \n";
    foreach $bench (keys %Benchmarks) {
       my @array = ();
      foreach $work (keys %WorkloadExp) {
        foreach $delta (keys %DeltaExp) {
          foreach $mut (keys %MutationExp) {
             $index = "$work:$delta:$mut:$bench";

		next if (not (defined $BenchmarkExp{$index}));
    		#print "$index ->  $BenchmarkExp{$index} \n";

    		$value = $BenchmarkExp{$index};
		push(@array, $value);
            }
        }
     }
        print " $bench: ";
	$min_value = min(@array);
	$max_value = max(@array);
	$avg_value = avg(@array);
        #printf "$min_value - $avg_value - $max_value";
        printf " %3.2f %3.2f\n", $avg_value/$min_value, $max_value/$min_value;
  }
}


sub summarizeBestWorkload
{
  print "Best workload\n";
      foreach $work (keys %WorkloadExp) {
       my @array = ();
        foreach $delta (keys %DeltaExp) {
          foreach $mut (keys %MutationExp) {
             $index = "$work:$delta:$mut";

                next if (not (defined $CPUExp{$index}));
                #print "$index ->  $BenchmarkExp{$index} \n";

                $value = $CPUExp{$index};
                push(@array, $value);
            }
     }
        print " $work: ";
        $min_value = min(@array);
        $max_value = max(@array);
        $avg_value = avg(@array);
        #printf "$min_value - $avg_value - $max_value";        
	printf " %3.2f %3.2f\n", $avg_value/$min_value, $max_value/$min_value;
  }
  print "\n";
      foreach $work (keys %WorkloadExp) {
        print " $work: \n";
        foreach $delta (keys %DeltaExp) {
          foreach $mut (keys %MutationExp) {
             $index = "$work:$delta:$mut";
	    printf "%3.2f ", $BenchmarkWorkloadReponse{$index};
            printf "%3.2f \n", $BenchmarkWorkloadTime{$index};
  }
       }
     }
}





sub summarizeResults
{
    summarizeBestDynamicBench();
    summarizeBestWorkload();
}


###############################################################################
## MAIN
###############################################################################
GetOptions (
  'log:s' => \$file,
  'max' => \$print_max,
  'bench:s' => \$bench,
  'cpu:s' => \$cpu,
  'static:s' => \$static,
  'stats' => \$print_stats,
  'summary' => \$summary,
  'help' => \$help);

# Ask for help
if(defined $help){
    help();
}

if (defined $bench) {
   readBenchSummaryFile($bench);

   if ($print_stats) {
      printBenchSummary($bench);
   }
}

if (defined $cpu) {
   readCPUSummaryFile($cpu);
   if ($print_stats) {
      printCPUSummary($cpu);
   }
}

if (defined $static) {
   readStaticBenchSummaryFile($static);
}

if ($summary) {
   summarizeResults();
}

