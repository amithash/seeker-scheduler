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
use Getopt::Long;
use File::Find;
use Cwd;

my $input_file_name;
my $output_dir;
my $process_name;
my $do_all = 0;
my %pid_hash;

GetOptions ('input:s' => \$input_file_name,
	    'output:s' => \$output_dir,
	    'name:s'  => \$process_name);

if(not defined($input_file_name)){
	print "-i flag is mandatory.\n";
	print "USAGE: pull.pl [-o /output/dir/name -n process_name] -i /path/to/input/file\n";
	exit(1);
}

if(not defined($output_dir)){
	$output_dir = cwd();
	print "Logs are in $output_dir\n";
}

if(not defined($process_name)){
	$do_all = 1;
}

open IN_FILE, "<$input_file_name" or die "Could not open $input_file_name, check path.\n";

my $line = <IN_FILE> | '';
my $header = $line;
my @infile = split(/\n/,join("",<IN_FILE>));
close(IN_FILE);

foreach $line (@infile){
	if($line =~ /^p,/){
		my @split_line = split(/,/,$line);
		if($do_all == 0){
			if($split_line[2] eq $process_name){
				if(not defined($pid_hash{$split_line[1]})){
					$pid_hash{$split_line[1]} = $split_line[2];
				}
			}
		}
		else{
			if(not defined($pid_hash{$split_line[1]})){
				$pid_hash{$split_line[1]} = $split_line[2];
			}
		}
	}
}



foreach my $pid_key (keys(%pid_hash)){
	my $core = -1;

	foreach $line (@infile){
		if($line =~ /^s,\d,$pid_key,/){
			my @split_line = split(/,/,$line);
			if($split_line[2] eq $pid_key){
				if($core == -1){
					$core = $split_line[1];
					my $log_file_name = "$output_dir/$pid_hash{$pid_key}.$pid_key.p$core.log";
					open OUT_FILE, "+>$log_file_name" or die "Could not create $log_file_name\n";
					print OUT_FILE "$header";
				}
				if($split_line[9] == 0 && $split_line[8] == 0 && $split_line[7] == 0){
					next;
				}
				my $new_line = join(',',@split_line[3, 5..$#split_line]);
				print OUT_FILE "$new_line\n";
			}
		}
	}
	close(OUT_FILE);
}


