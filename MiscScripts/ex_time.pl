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
use lib "$ENV{SEEKER_HOME}/lib";
use benchmarks;

if($#ARGV != 1){
	print "Usage: $0 <PATH TO TIME FILE> <PATH TO OUTPUT FILE>\n";
	exit;
}

my $inf = $ARGV[0];
my $outf = $ARGV[1];

open IN,"$inf" or die "Could not open $inf\n";
open OUT,"+>$outf" or die "Could not create $outf\n";

while(my $line = <IN>){
	chomp($line);
	if($line =~ /^"(.+)"\s+(\d+\.\d+)$/){
		my $bench_path = $1;
		my $time = $2;
		my @temp = split(/\//,$bench_path);
		my $bin_name = $temp[$#temp];
		my $bench = benchmarks::get_bench_name($bin_name);
		print OUT "$bench $time\n";
	} else {
		print "Warning! unrecognized line\n";
	}
}
close(IN);
close(OUT);
