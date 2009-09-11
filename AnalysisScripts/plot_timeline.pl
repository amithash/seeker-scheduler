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

my $input;
my $output;
my $type;

GetOptions(
	'in:s' => \$input,
	'out:s' => \$output,
	'type:s' => \$type
);
if(defined($type)){
	$type = "-type $type";
} else {
	$type = "";
}

if(not defined($output)){
	$output = "../figures/temp";
}
if(not defined($input)){
	print "Bad input: $input\n";
	exit;
}

unless(-d $input){
	print "$input does not exist\n";
	exit;
}
system("mkdir -p $output") unless(-d $output);
my @infiles = <$input/log_*>;
foreach my $dir (@infiles){
	if($dir =~/log_(\d_\d_\d_\d_\d)_(\S+)$/){
		my $layout = $1;
		my $group = $2;
		system("./plot_bench.pl $type -in $dir -out $output/$layout/$group");
	} elsif($dir =~ /log_(\d+)_(\d)_(\S+)$/){
		my $interval = $1;
		my $delta = $2;
		my $group = $3;
		system("./plot_bench.pl $type -in $dir -out $output/bench/$interval/$delta/$group");
		system("./plot_mutate.pl $type -in $dir -out $output/mutate/$interval/$delta/$group");
	}
}

