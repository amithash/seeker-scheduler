#!/usr/bin/perl

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

GetOptions( 'b|bench=s{,}' => \@bench ,
	    'benchlist=s' => \$bench_list,
	    'a|any'     => \$any,
	    'l|log=s'	=> \$log,
	    'prefix=s'    => \$pre_ex,
	    'postfix=s'	=> \$post_ex,
	    'finalize=s' => \$finalize,
	    'seq=i'	=> \$seq,
	    'o|output=s' => \$output);

$output = "run.sh" unless(defined($output));
$any = 0 unless(defined($any));
$log = "LOG" unless(defined($log));
$pre_ex = " " unless(defined($pre_ex));
$post_ex = " " unless(defined($post_ex));
$finalize = " " unless(defined($finalize));
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
		my $mask = (1 << $nr_cpus) - 1;
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

