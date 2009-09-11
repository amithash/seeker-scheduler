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

my $ipc_stat = 0;
my $state_stat = 0;
my $threshold;
my $level;

GetOptions( 'i|ipc-stat' => \$ipc_stat,
	    's|state-stat' => \$state_stat,
	    't|threshold=f' => \$threshold,
	    'l|level=f'  => \$level);

if(not defined($threshold)){
	$threshold = 1.0;
}

$level = 0.7 unless(defined($level));

if($ipc_stat == 0 and $state_stat == 0){
	$ipc_stat = $state_stat = 1;
}

my %stats = (	'low' => 0,
   		'high' => 0
	);
my %residency;

if($#ARGV != 0){
	print "USAGE: $0 file";
	exit;
}

my $total = 0.0;

open IN, "$ARGV[0]" or die "Opening $ARGV[0] failed with $!\n";
while(my $line = <IN>){
	chomp($line);
	my @l = split(/\s/,$line);
	my $ipc = $l[6];
	my $cy = $l[4];
	my $state = $l[7];
	if($cy > 100000000000.0){
		print STDERR "$ARGV[0] warn\n";
		next;
	}
	if($ipc_stat == 1){
		classify($ipc,$cy * 1.0);
	}
	if($state_stat == 1){ 
		residency($state,$cy * 1.0);
	}
	$total += ($cy*1.0);
}
if($ipc_stat == 1){
	my $header = "";
	my $data = "";
	foreach my $ipc ("low","high"){
		$header = $header . "$ipc ";
		if($stats{$ipc} < 1.0){
			$data = $data . "0.0 ";
			next;
		}
		my $perc = $stats{$ipc} * 100 / $total;
		if($perc < $threshold){
			$data = $data . "0.0 ";
			next;
		}
		my $perc_s = sprintf("%.2f",$perc);
		$data = $data . "$perc_s ";
	}
	print "$header\n";
	print "$data\n";
}

if($state_stat == 1){
	foreach my $state (sort keys %residency){
		my $cyb = $residency{$state} / (10.0 ** 9);
		my $cyb_s = sprintf("%.4f",$cyb);
		print "$state\t$cyb_s\n";
	}
}

sub residency{
	my $state = shift;
	my $cy = shift;
	if(defined($residency{$state})){
		$residency{$state} += $cy;
	} else {
		$residency{$state} = $cy;
	}
}

sub classify{
	my $ipc = shift;
	my $cy = shift;

	if($ipc <= $level){
		$stats{'low'} += $cy;
		return;
	}
	if($ipc > $level ){
		$stats{'high'} += $cy;
		return;
	}
	#should never be executed.
	print "error";
}

