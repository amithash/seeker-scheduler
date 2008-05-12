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

die "USAGE: mother_auto.pl /path/to/binary/files /path/to/output/dirs EXECUTION_LOG 
INTERPOLATION_INTERVAL(Millions) SMOOTHNING_WINDOW (-paired | -single) 
[-all | -pull | -org | -interp | -mm | -smooth | -conv | -graph]\n" unless ($#ARGV == 6);
my $input = $ARGV[0];
my $output = $ARGV[1];
my $interval = $ARGV[3] + 0;
my $window = $ARGV[4] + 0;
my $paired = -1;
my $execution = $ARGV[2];
my $what = $ARGV[6];
die "$execution does not exist\n" unless(-e $execution);

$paired = 0 unless ($ARGV[5] eq "-paired");
$paired = 1 unless ($ARGV[5] eq "-single");

# Handle errors in the input....
die "The 5th argument should be either -paired or -single, provided parameter: $ARGV[5]\n" unless ($paired != -1);
die "$input does not exist or is not a dir\n" unless(-d "$input");
die "INTERPOLATION INTERVAL has to be greater than 0\n" unless($interval > 0);
die "Smoothing window has to be greater than 0\n" unless($window > 0);
die "SEEKER_HOME env variable is not defined.\n" unless(defined($ENV{SEEKER_HOME}));
die "Please compile seeker\n" unless(-e "$ENV{SEEKER_HOME}/Scripts/interp");
die "Please compile seeker\n" unless(-e "$ENV{SEEKER_HOME}/Scripts/smooth");
die "Please compile seeker\n" unless(-e "$ENV{SEEKER_HOME}/Scripts/maxmin");
system("mkdir $output") unless (-d "$output");

my $misc_scripts = "$ENV{SEEKER_HOME}/MiscScripts";
my $scripts = "$ENV{SEEKER_HOME}/Scripts";

system("$misc_scripts/auto_pulldecode.pl -i $input -o $output/pulled --delete") if($what eq "-all" or $what eq "-pull");
system("$misc_scripts/pre_organize.pl $execution $output/pulled $output/organized $ARGV[5]") if($what eq "-all" or $what eq "-org");
system("mkdir $output/interp") unless (-d "$output/interp");
seeker::do_per_dir("$output/organized", "$output/organized", "$output/interp", 
		  "$ENV{SEEKER_HOME}/Scripts/interp INPUT OUTPUT $interval") 
		  if($what eq "-all" or $what eq "-interp");
if($paired){
	system("$misc_scripts/auto_maxmin.pl $output/interpolated $output/maxmin") if($what eq "-mm" or $what eq "-all");
	seeker::do_per_dir("$output/maxmin", "$output/maxmin", "$output/smooth", 
			   "$ENV{SEEKER_HOME}/Scripts/smooth INPUT OUTPUT $window")
		           if($what eq "-all" or $what eq "-smooth");
}
else{
	seeker::do_per_dir("$output/interp", "$output/interp", "$output/smooth", 
			   "$ENV{SEEKER_HOME}/Scripts/smooth INPUT OUTPUT $window")
		           if($what eq "-all" or $what eq "-smooth");
}
seeker::do_per_dir("$output/smooth", "$output/smooth", "$output/smooth_tsv", 
		   "$ENV{SEEKER_HOME}/Scripts/scv2tsv.pl INPUT OUTPUT")
		   if($what eq "-all" or $what eq "-conv");
# PLOTS ANYONE? 

