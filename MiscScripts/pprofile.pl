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


# NOT USED
# Requires support from the test system to allow Power or at least
# Voltage and Current reading from the IPMI interface.

use strict;
use warnings;

my $infile = $ARGV[0];
my $outfile = $ARGV[1];

open IN, "$infile" or die "Could not open $infile\n";
open OUT,"+>$outfile" or die "Could not open $outfile\n";
my @cont = <IN>;
close(IN);
chomp($cont[0]);
my @temp = split(/,/,$cont[0]);
my $start = int($temp[0]);
my $last = $start;
my $count = 1;
my $sum = $temp[1] * 1.0;
shift @cont;

foreach my $line (@cont){
	chomp($line);
	my @t = split(/,/,$line);
	my $time = int($t[0]);
	my $pow = $t[1] + 0.0;
	if($time == $last){
		$sum = $sum + $pow;
		$count = $count + 1;
	} else {
		my $p = $sum / ($count * 1.0);
		my $tt = $last - $start;
		print OUT "$tt $p\n";
		$last = $time;
		$sum = $pow;
		$count = 1;
	}
}
my $p = $sum / ($count * 1.0);
my $tt = $last - $start;
print OUT "$tt $p\n";
close(OUT);


