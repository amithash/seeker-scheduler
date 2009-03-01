#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;
use Time::HiRes;

my $s_pattern;
my $interval;
GetOptions('p|pattern=s'   => \$s_pattern,
	   'i|interval=i'  => \$interval);

# Defaults.
$s_pattern = "0000" unless(defined($s_pattern));
$interval = 1000 unless(defined($interval));

# Get real pattern. 
my @pattern = split(//,$s_pattern);

# Get total online cpus.
my $total_cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($total_cpus);
$total_cpus = int($total_cpus);

# Get max states a core can have.
my $max = get_max_states();

# validate pattern.
for(my $i=0;$i < scalar @pattern; $i++){
	if($pattern[$i] =~ /^\d$/){
		$pattern[$i] = int($pattern[$i]);
	} else {
		print "Pattern $s_pattern contains non-numeric values\n";
		exit;
	}
	if($pattern[$i] >= $max){
		print "$pattern[$i] (${i}th from the left) is not a valid state\n";
		print "Valid states are in the interval [0,$max)\n";
		exit;
	}
}

#Kernel. 
while(1){
	foreach my $p (@pattern){
		set_speed_all($p);
		usleep($interval * 1000);
	}
}

sub get_max_states{
	my $l = `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`;
	chomp($l);
	my @a = split(/\s+/,$l);
	return scalar @a;
}

sub set_speed_all{
	my $state = shift;
	for(my $i=0;$i<$total_cpus;$i++){
		set_speed($i,$state);
	}
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


