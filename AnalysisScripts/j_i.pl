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
my $states = 5;
my $bench_f;
GetOptions('s|state=i' => \$states,
	   'i|in=s'    => \$bench_f);

if($states != 2 and $states != 5){
	print "Error only states 2 and 5 are supported\n";
	exit;
}
if(not defined($bench_f)){
	usage();
}

open IN, "$bench_f" or die "Could not open $bench_f\n";

my %energy;
my %inst;
my %pow;
my %count;
my %time;

my @PowerValue = (64.6);
if($states == 5){
	push @PowerValue, 75.6;
	push @PowerValue, 88.8;
	push @PowerValue, 108.2;
	push @PowerValue, 115.0;
	
} else {
	push @PowerValue, 115.0;
}
my $header;
while(my $line = <IN>){
	chomp($line);
	my @ln = split(/\s/,$line);
	my $exp = $ln[0];
	my $bench = $ln[1];
	my $experiment;
	if($exp =~ /log_(\d+)_(\d+)_([A-Za-z-]+)$/){
		my $int = $1;
		my $dlt = $2;
		my $grp = $3;
		$experiment = "$grp $int $dlt";
		if(not defined($header)){
			$header = "workload interval delta";
		}
	} elsif($exp =~ /log_(\d+)_([A-Za-z-]+)$/){
		my $int = $1;
		my $grp = $2;
		$experiment = "$grp $int";
		if(not defined($header)){
			$header = "workload interval";
		}
	} elsif($exp =~ /log_(\d)_(\d)_(\d)_(\d)_(\d)_([A-Za-z-]+)$/){
		my $p0 = $1;
		my $p1 = $2;
		my $p2 = $3;
		my $p3 = $4;
		my $p4 = $5;
		my $group = $6;
		$experiment = "$group [$p0,$p1,$p2,$p3,$p4]";
		if(not defined($header)){
			$header = "workload layout";
		}
	} else {
		print "Not working\n";
		next;
	}
	
	my $cy = $ln[2];
	my $ins = $ln[3];
	my $tm = $ln[$#ln];
	my $egy = $ln[$#ln-1];
		
	my $avg_pow = 0.0;
	for(my $i = 4; $i < $#ln - 1; $i++){
		my $p = $PowerValue[$i - 4];
		my $perc = $ln[$i];
		$avg_pow += ($p * $perc);
	}

	if(defined($energy{$experiment})){
		$energy{$experiment} += $egy;
		$inst{$experiment} += $ins;
		$pow{$experiment} += $avg_pow;
		$time{$experiment} += $tm;
		$count{$experiment} += 1;
	} else {
		$energy{$experiment} = $egy;
		$inst{$experiment} = $ins;
		$pow{$experiment} = $avg_pow;
		$time{$experiment} = $tm;
		$count{$experiment} = 1;
	}
}
# Print header
print "$header jpbi avgpwr time\n";

foreach my $experiment (sort keys %energy){
	my $jpbi = $energy{$experiment} / ($inst{$experiment} / (10**9));
	my $avg_pow = $pow{$experiment} / $count{$experiment};
	my $tot_time = $time{$experiment};
	printf("%s %.4f %.4f %.4f\n",$experiment, $jpbi,$avg_pow,$tot_time);
}

sub usage
{
	print "USAGE: $0 -i <input bench file> [-s <total states>, default=5]\n";
	exit;
}

