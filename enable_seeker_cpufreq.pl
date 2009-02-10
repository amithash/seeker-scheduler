#!/usr/bin/perl

use strict;
use warnings;

my $cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($cpus);

if($#ARGV < 0){
	print "USAGE: $0 [start|stop]\n";
}
if(not($ARGV[0] !~ /[Ss][Tt][Aa][Rr][Tt]/ or $ARGV[0] !~ /[Ss][Tt][Oo][Pp]/)){
	print "USAGE: $0 [start|stop]\n";
}


print "$cpus\n";

for(my $i=0;$i<$cpus;$i++){
	if($ARGV[0] =~ /[Ss][Tt][Aa][Rr][Tt]/){
		system("echo \"seeker\" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
	} elsif($ARGV[0] =~ /[Ss][Tt][Oo][Pp]/){
		system("echo \"performance\" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
	}
}

