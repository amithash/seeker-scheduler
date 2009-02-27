#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;

my $s_pattern;
GetOptions('p|pattern=s'   => \$s_pattern);

my @pattern = split(//,$s_pattern);

# validate.
for(my $i=0;$i < scalar @pattern; $i++){
	if($pattern[$i] =~ /^\d$/){
		$pattern[$i] = int($pattern[$i]);
	} else {
		print "Pattern $s_pattern contains non-numeric values\n";
		exit;
	}
}
my $max = get_max_states();

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


