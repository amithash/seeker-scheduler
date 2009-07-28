#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;

my $input;
my $output;
my $type;

GetOptions(
	'in:s' => \$input,
	'out:s' => \$output,
	'type:s' => \$type
);
if(defined($type)){
	$type = "-type $type";
} else {
	$type = "";
}

if(not defined($output)){
	$output = "../figures/temp";
}
if(not defined($input)){
	print "Bad input: $input\n";
	exit;
}

unless(-d $input){
	print "$input does not exist\n";
	exit;
}
system("mkdir -p $output") unless(-d $output);
my @infiles = <$input/log_*>;
foreach my $dir (@infiles){
	if($dir =~/log_(\d_\d_\d_\d_\d)_(\S+)$/){
		my $layout = $1;
		my $group = $2;
		system("./plot_bench.pl $type -in $dir -out $output/$layout/$group");
	} elsif($dir =~ /log_(\d+)_(\d)_(\S+)$/){
		my $interval = $1;
		my $delta = $2;
		my $group = $3;
		system("./plot_bench.pl $type -in $dir -out $output/bench/$interval/$delta/$group");
		system("./plot_mutate.pl $type -in $dir -out $output/mutate/$interval/$delta/$group");
	}
}

