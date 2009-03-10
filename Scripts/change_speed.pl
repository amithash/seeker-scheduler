#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;

my @pattern;
GetOptions('p|pattern=i@{1,}'   => \@pattern);

# Get total online cpus.
my $total_cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($total_cpus);
$total_cpus = int($total_cpus);

# Get max states a core can have.
my $max = get_max_states();

if(scalar(@pattern > $total_cpus)){
		print "ERROR, total entries are more than there are cpus\n";
		exit;
}

# validate pattern.
for(my $i=0;$i < scalar @pattern; $i++){
	if($pattern[$i] =~ /^\d$/){
		$pattern[$i] = int($pattern[$i]);
	} else {
		print "Pattern[$i] = $pattern[$i] contains non-numeric values\n$usage";
		exit;
	}

	if($pattern[$i] >= $max or $pattern[$i] < 0){
		print "Pattern[$i] = $pattern[$i] is not a valid state\n";
		print "Valid states are in the interval [0,$max)\n$usage";
		exit;
	}
}

# Kernel single proc
for(my $i=0;$i < scalar(@pattern); $i++){
	set_speed($i,$pattern[$i]);
}

sub get_max_states{
	my $l = `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`;
	chomp($l);
	my @a = split(/\s+/,$l);
	return scalar @a;
}

sub set_speed{
	my $cpu = shift;
	my $state = shift;

	my $f = "/sys/devices/system/cpu/cpu$cpu/cpufreq/scaling_setspeed";
	unless(-e $f){
		return 0;
	}
	return system("echo \"$state\" > $f");
}


