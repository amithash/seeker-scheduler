#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: powersample.pl
 # DESCRIPTION: samples the power readings taken from the IPMI interface.
 # This is too specific to be here, but a useful start on how to do it.
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

if($#ARGV != 0){
	print "USAGE: $0 FILE\n";
	exit;
}
my $ipmi = `which ipmitool`;
my $date = `which date`;
chomp($ipmi);
chomp($date);
if(! -e $ipmi || !-e $date){
	die "ipmitool is not installed\n";
}

my $Voltage = "PS0/V_OUT";
my $Current = "PS0/I_OUT";
my $outfile = $ARGV[0];
system("ipmitool sdr dump local.sdr");


my $pid = fork();
if($pid != 0){
	exit;
}

my $first = -1;

open OUT, "+>$outfile" or die "Could not open or create $outfile\n";
while(1){
	my $current = 0;
	my $voltage = 0;
	my $power = 0;
	open IN, "$ipmi -S local.sdr sensor get $Voltage $Current | grep \"Sensor Reading\" |" or warn "Could not execute $ipmi\n";
	my @temp = <IN>;
	close(IN);
	foreach my $entry (@temp){
		if($entry =~ /Sensor Reading\s+\:\s+(\d+\.\d+)\s+.+Volts/){
			$voltage = $1 + 0.0;
		} elsif($entry =~ /Sensor Reading\s+\:\s+(\d+\.\d+)\s+.+Amps/){
			$current = $1 + 0.0;
		} elsif($entry =~ /Sensor Reading\s+\:\s+(\d+)\s+.+Amps/){
			$current = $1 + 0.0;
		} elsif($entry =~ /Sensor Reading\s+\:\s+(\d+)\s+.+Volts/){
			$voltage = $1 + 0.0;
		} else {
			next;
		}
	}
	
	if($voltage == 0 or $current == 0){
		my $joined_entry = join("",@temp);
		$joined_entry =~ s/\n//g;
		print "Something is wrong ($joined_entry)\n";
		next;
	}
	$power = $voltage * $current;
	my $interval = `/bin/date +"%s"`;
	chomp($interval);
	if($first == -1){
		$first = int($interval);
		print OUT "0,$power\n";
	} else {
		my $int = int($interval) - $first;
		print OUT "$int,$power\n";
	}
	sleep 1;
}
close(OUT);
close(IN);

