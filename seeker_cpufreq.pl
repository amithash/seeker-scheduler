#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: seeker_cpufreq.pl
 # DESCRIPTION: In order for seeker_scheduler to use seeker_cpufreq, it must
 # be selected as the default governor for _all_ cpus. Instead of doing this
 # manually by going to the sysfs and enabling it on one cpu at a time, this
 # script does that for you.
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

my $cpus = `cat /proc/cpuinfo | grep MHz | wc -l`;
chomp($cpus);

if($#ARGV < 0){
	print "USAGE: $0 [start|stop]\n";
}
if(not($ARGV[0] !~ /[Ss][Tt][Aa][Rr][Tt]/ or $ARGV[0] !~ /[Ss][Tt][Oo][Pp]/)){
	print "USAGE: $0 [start|stop]\n";
}


for(my $i=0;$i<$cpus;$i++){
	if($ARGV[0] =~ /[Ss][Tt][Aa][Rr][Tt]/){
		system("echo \"seeker\" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
		system("cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
	} elsif($ARGV[0] =~ /[Ss][Tt][Oo][Pp]/){
		system("echo \"performance\" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
		system("cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
	}
}

