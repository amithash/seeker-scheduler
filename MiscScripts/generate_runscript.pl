#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: generate_runscript.pl
 # DESCRIPTION: This generates a run script from a benchlist to run a set of
 # of benchmarks from the IBS (Iyyer benchmark suite) or otherwise known as
 # the DRACO benchmark suite. The suite is a collection of benchmark suites
 # like the SPEC suites which is re-organized in a nice consistent way. If you
 # are not in CU or work under members of the system/Draco/Axion lab then 
 # this script is next to useless to you.  
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

use strict;
use warnings;
use Getopt::Long;
use lib "$ENV{SEEKER_HOME}/lib";
use benchmarks; 

my @bench;
my $nr_cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($nr_cpus);
$nr_cpus = int($nr_cpus);

my $any;
my $log;
my $pre_ex;
my $post_ex;
my $seq;
my $finalize;
my $bench_list;
my $output;
my $req_mask;
my $helpf;

GetOptions( 'b|bench=s{,}' => \@bench ,
	    'benchlist=s' => \$bench_list,
	    'm|mask=i'     => \$req_mask,
	    'a|any'        => \$any,
	    'l|log=s'	=> \$log,
	    'prefix=s'    => \$pre_ex,
	    'postfix=s'	=> \$post_ex,
	    'finalize=s' => \$finalize,
	    'seq=i'	=> \$seq,
	    'help'	=> \$helpf,
	    'o|output=s' => \$output);

if(defined($helpf)){
	help();
}

$output = "run.sh" unless(defined($output));
$any = 0 unless(defined($any));
$log = "LOG" unless(defined($log));
$pre_ex = " " unless(defined($pre_ex));
$post_ex = " " unless(defined($post_ex));
$finalize = " " unless(defined($finalize));

#TEST
my $mask = (1 << $nr_cpus) - 1;

if(defined($req_mask)){
	if($req_mask < $mask){
		$mask = $req_mask;
	}
	$any = 1;
}

if(defined($seq)){
	$seq = 0 unless ($seq < $nr_cpus and $seq >= 0);
	$seq = 1 << $seq;
}

if(defined($bench_list)){
	open BL, "$bench_list" or die "Could not open the benchlist = $bench_list. $!\n";
	chomp(@bench = <BL>);
	close(BL);
}

my $s = "";
my $c = "";
my @run = ();

my %cs_hash;

foreach my $b (@bench){
	if(not defined($cs_hash{$b})){
		$c = $c . benchmarks::cleanup($b) . "\n";
		$s = $s . benchmarks::setup($b) . "\n";
		$cs_hash{$b} = 1;
	}
	my $rn = benchmarks::run($b);
	if((not defined($rn)) or $rn eq ""){
		print "$b is not a valid benchmark. exiting.\n";
		exit;
	}
	push @run,$rn;
}
my $cpu = 0;
my $launch = "/usr/bin/launch";
my $run_cmd = "";
$run_cmd = $launch unless(defined($seq));
foreach my $r (@run){
	if(defined($seq)){
		$run_cmd .= $pre_ex . "\n" . $launch . " " . "core $seq " . "\"$r\"" . " >> $log\n" . $post_ex . "\n";
		next;
	}
	if($any == 1){
		$run_cmd = $run_cmd . " core $mask \"$r\"";
	} else {
		my $mask = (1 << $cpu);
		$run_cmd = $run_cmd . " core $mask \"$r\"";
		$cpu = $cpu < $nr_cpus ? $cpu + 1 : 0;
	}
}
$run_cmd = "$pre_ex\n$run_cmd >> $log\n$post_ex\n" unless(defined($seq));

open R,"+>$output" or die "Could not create run.sh\n";
print R "#!/bin/sh\n\n";
print R "$s\n\n";
print R "$run_cmd\n\n";
print R "$finalize\n\n";
print R "$c\n\n";

close(R);
system("chmod +x $output");

sub help
{
	print "
	USAGE:
	$0 [OPTIONS]
	OPTIONS:
	-b|--bench=b1,b2,b3,... :  list of benchmarks.
	--benchlist=bl          :  use benchlist bl
				   bl is of the format:
				   b1
				   b2
				   b3
				   ...
	-m|--mask=i             :  The cpu mask on which to execute.
	-a|--any		:  run on any cpu.
	-l|--log=lg		:  log to fileL lg
	--prefix=px		:  prefix px to command.
	--postfix=px		:  append px to command.
	--finalize=s		:  do 's' when done.
	--seq=i			:  run benchmarks sequentially on cpu i.
	--output=s		:  the path to the resulting run script.
				   it is run.sh in the current dir by default.
	
	\n";
	exit;
}

