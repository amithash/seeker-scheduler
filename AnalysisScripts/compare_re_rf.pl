#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: compare_re_rf.pl
 # DESCRIPTION: Read in a gzipped sch file and then printout state reference
 # IPC and real IPC. Later this data can be used to plot throughput vs 
 # IPC for each performance state. 
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

unless(defined($ARGV[0])){
	print "Need input file\n";
	exit;
}
my $inf = $ARGV[0];

open IN, "cat $inf | gunzip |" or die "Could not open $inf!\n";
while(my $line = <IN>){
	chomp($line);
	my @tmp = split(/\s/, $line);
	my $inst = $tmp[3];
	my $rfcy = $tmp[4];
	my $reipc = $tmp[6];
	my $st = $tmp[7];
	if($rfcy > (50 * 10**9)){
		next;
	}
	my $rfipc = $inst / $rfcy;
	printf("%d %.3f %.3f\n", $st, $rfipc, $reipc);
}

