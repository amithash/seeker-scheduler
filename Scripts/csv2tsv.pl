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

die "USAGE: csv2tsv.pl /path/to/input /path/to/output\n" unless($#ARGV == 1);
my $input = $ARGV[0];
my $output = $ARGV[1];
die "$input does not exist\n" unless (-e "$input");

open IN, "$input" or die "$input could not be opened\n";
open OUT, "+>$output" or die "$output could not be created\n";
my $line;
while($line = <IN>){
	chomp($line);
	if($line =~ /\D+,/){
		print OUT "# $line\n";
	}
	else{
		$line =~ s/,/ /g;
		print OUT "$line\n";
	}
}
close(IN);
close(OUT);
