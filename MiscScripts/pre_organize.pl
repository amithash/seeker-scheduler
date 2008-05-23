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

die "USAGE: ./org.pl EXECUTION_LOG /path/to/input/dir /path/to/output/dir -paired|-single\n" unless($#ARGV == 3);
my $exec_path   = $ARGV[0];
my $input_path  = $ARGV[1];
my $output_path = $ARGV[2];
my $paired = $ARGV[3];
my %log;
my %file_name1;
my %file_name2;
my %sub_path1;
my %sub_path2;

die "$input_path does not exist or is not a dir\n" unless(-d "$input_path");
open EXEC, "$exec_path" or die "Could not open $exec_path to read\n";
my @exec_order = split(/\n/, join("",<EXEC>));
close(EXEC);
my $index = 0;
my $logname = "log";
system("mkdir $output_path") unless (-d "$output_path");

foreach my $execution (@exec_order){
	my $logn = $logname . $index;
	if($paired eq "-paired"){
		if($execution =~ /Started : spec-cpu2006\/\D+\/(\d+\..+), input: ref (\d) \+ spec-cpu2006\/\D+\/(\d+\..+), input: ref (\d)/){
			$log{"$logn"} = "$1:$3";
			my $name1 = $3 . "_ref" . $4 . "_p1.log";
			my $name2 = $1 . "_ref" . $2 . "_p0.log";
			$sub_path1{"$logn"} = "ref$2/p0";
			$sub_path2{"$logn"} = "ref$4/p1";
			$file_name1{"$logn"} = "$name1";
			$file_name2{"$logn"} = "$name2";
			$index = $index + 1;
		}
	}
	elsif($paired eq "-single"){
		if($execution =~ /Started : spec-cpu2006\/\D+\/(\d+\..+), input: ref (\d)/){
			$log{"$logn"} = "$1:$1";
			$file_name1{"$logn"} = "single.log";
			$file_name2{"$logn"} = "single.log";
			$sub_path1{"$logn"} = "ref$2/p0";
			$sub_path2{"$logn"} = "ref$2/p1";
			$index = $index + 1;
		}
	}
	else{
			die "The 4th arg can either be -paired or -single, $paired provided\n";
	}
}

my @logs = seeker::get_dir_tree($input_path);
my $count1 = 0;
my $count2 = 0;
my $count_1 = 0;
my $count_2 = 0;
foreach my $logp (@logs){
	next unless ($logp ne "");
	$count1++;
	die "file_name1 for $logp is not defined\n" unless (defined($file_name1{$logp}));
	if($paired eq "-paired"){
		die "file_name2 for $logp is not defined\n" unless (defined($file_name2{$logp}));
	}
	die "log for $logp is not defined\n" unless (defined($log{$logp}));
	
	my $error = 1;
	$error = 2 unless($paired eq "-single");
	my @bins = split(/:/, $log{$logp});
	die "bins does not contain 2! bins contains $#bins\n" unless ($#bins == 1);

	my @bnames = ("", "");
	my @recorded = seeker::get_dir_tree("$input_path/$logp");
	foreach my $record (@recorded){
		$count2++;
		if($record =~ /$bins[0]\.\d+\.p0\.log/){
			$count_1++;
			$error = $error - 1;
			$bnames[0] = $record unless ($bnames[0] ne "");
			print "Found $record for $bins[0] in $logp\n";
		}
		if($record =~ /$bins[1]\.\d+\.p1\.log/){
			$count_2++;
			$error = $error - 1;
			$bnames[1] = $record unless ($bnames[1] ne "");
			print "Found $record for $bins[1] in $logp\n";
		}
	}
	die "ERROR, your execution log does not match with this dir\ndir: $logp, $log{$logp}" unless ($error == 0);
	if($bnames[0] ne ""){
		seeker::make_dir("$output_path/$bins[0]/$sub_path1{$logp}");
		system("cp $input_path/$logp/$bnames[0] $output_path/$bins[0]/$sub_path1{$logp}/$file_name1{$logp}");
	}
	if($bnames[1] ne ""){
		seeker::make_dir("$output_path/$bins[1]/$sub_path2{$logp}");
		system("cp $input_path/$logp/$bnames[1] $output_path/$bins[1]/$sub_path2{$logp}/$file_name2{$logp}");
	}
}
print "$count1\n$count2\n$count_1\n$count_2\n";
