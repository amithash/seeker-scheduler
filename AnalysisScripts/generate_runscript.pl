#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;
use lib "$ENV{SEEKER_HOME}/AnalysisScripts";
use benchmarks; 

my @bench;
my $nr_cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($nr_cpus);
$nr_cpus = int($nr_cpus);

my $any;
my $log;
GetOptions( 'b|bench=s{,}' => \@bench ,
	    'a|any'     => \$any,
	    'l|log=s'	=> \$log);
$any = 0 if(not defined($any));
$log = "LOG" if(not defined($log));

my $br = $ENV{BENCH_ROOT};

my $s = "";
my $c = "";
my @run = ();

foreach my $b (@bench){
	$c = $c . benchmarks::cleanup($b,"$br/bench") . "\n";
	$s = $s . benchmarks::setup($b,"$br/bench") . "\n";
	my $rn = benchmarks::run($b,"$br/bench");
	next if(not defined($rn));
	next if($rn eq "");
	push @run,$rn;
}
my $cpu = 0;
my $run_cmd = "/usr/bin/launch";
foreach my $r (@run){
	if($any == 1){
		my $mask = (1 << $nr_cpus) - 1;
		$run_cmd = $run_cmd . " core $mask \"$r\"";
	} else {
		my $mask = (1 << $cpu);
		$run_cmd = $run_cmd . " core $mask \"$r\"";
		$cpu = $cpu < $nr_cpus ? $cpu + 1 : 0;
	}
}
$run_cmd = $run_cmd . " >> $log\n";

open R,"+>run.sh" or die "Could not create run.sh\n";
print R "#!/bin/sh\n\n";
print R "$s\n\n";
print R "$run_cmd\n\n";
print R "$c\n\n";

close(R);

