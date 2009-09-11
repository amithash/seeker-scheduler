#!/usr/bin/perl
#*************************************************************************
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

use strict;
use warnings;
use Getopt::Long;
use Time::HiRes qw( usleep );

my $usage = "USAGE: $0 -p S0 S1 S2 -i I0 I1 I2 [-c processor_number]\nExample: $0 -p 0 4 -i 50 100\nWill have the effect State_0 (For 50ms) State_4 (for 100ms) and back\n";
my @pattern;
my @interval;
my $core;
GetOptions('p|pattern=i@{1,}'   => \@pattern,
	   'i|interval=i@{1,}'  => \@interval,
	   'c|core=i'      => \$core);

# Get total online cpus.
my $total_cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($total_cpus);
$total_cpus = int($total_cpus);

# Get max states a core can have.
my $max = get_max_states();

# Validate parameters; 

if(scalar(@pattern) == 0 or scalar(@interval) == 0){
	print "Required Parameters not provided (-p and -i)\n$usage";
	exit;
}

if($#pattern != $#interval){
	print "Unequal pattern and interval.\n$usage";
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
	if($interval[$i] =~ /^\d+$/){
		$interval[$i] = int($interval[$i]);
	} else {
		print "Interval[$i] = $interval[$i] contains non-numeric values\n$usage";
		exit;
	}

	if($pattern[$i] >= $max or $pattern[$i] < 0){
		print "Pattern[$i] = $pattern[$i] is not a valid state\n";
		print "Valid states are in the interval [0,$max)\n$usage";
		exit;
	}
}

# validate core.
if(defined($core)){
	if($core < 0 or $core >= $total_cpus){
		print "-c|--core option has an invalid processor, valid options=[0,$total_cpus)\n$usage";
		exit;
	}
}

# Daemonize.
my $pid = fork;
if($pid != 0){
	exit;
}

#Kernel all procs
if(not defined($core)){
	while(1){
		for(my $i=0;$i < scalar(@pattern); $i++){
			set_speed_all($pattern[$i]);
			usleep($interval[$i] * 1000);
		}
	}
} 

# Kernel single proc
while(1){
	for(my $i=0;$i < scalar(@pattern); $i++){
		set_speed($core,$pattern[$i]);
		usleep($interval[$i] * 1000);
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


