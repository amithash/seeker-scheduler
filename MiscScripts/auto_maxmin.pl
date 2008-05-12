#!/usr/bin/perl
#*************************************************************************
# Copyright 2008 Amithash Prasad                                         *
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
use lib "$ENV{SEEKER_HOME}/Scripts";
use seeker;

die "Usage: ./maxmin.pl /path/to/input/dir /path/to/output/dir\n" unless($#ARGV == 1);
die "Please compile seeker. or set SEEKER_HOME env variable\n" unless(-e "$ENV{SEEKER_HOME}/Scripts/maxmin");
my $input_path = $ARGV[0];
my $output_path = $ARGV[1];

my @dirs1 = seeker::get_dir_tree($input_path);

# BENCHES
system("mkdir $output_path") unless (-d "$output_path");
foreach my $dir1 (@dirs1){
	my @dirs2 = seeker::get_dir_tree("$input_path/$dir1");

	# REF
	foreach my $dir2 (@dirs2){
		my @dirs3 = seeker::get_dir_tree("$input_path/$dir1/$dir2");

		# P
		foreach my $dir3 (@dirs3){
			my @files = get_dir_tree("$input_path/$dir1/$dir2/$dir3");
			for(my $i=0;$i<=$#files;$i = $i+1){
				$files[$i] = "$input_path/$dir1/$dir2/$dir3/$files[$i]";
			}
			my $list = join(" ", @files);

			system("$ENV{SEEKER_HOME}/Scripts/maxmin $output_path/$dir1\_$dir2\_$dir3 $list");
		}
	}
}

