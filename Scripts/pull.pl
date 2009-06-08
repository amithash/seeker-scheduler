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
use lib "$ENV{SEEKER_HOME}/lib";
use benchmarks;

my $input_file_name;
my $output_dir;
my $bench;
my $do_all = 0;
my %pid_hash;
my $what;
my $benchlist;
my $total_online_cpus = 4;

GetOptions ('input:s' => \$input_file_name,
	    'output:s' => \$output_dir,
	    'b|bench:s'  => \$bench,
	    'what:s'  => \$what,
	    'cpus:i'  => \$total_online_cpus,
	    'benchlist:s'  => \$benchlist);
$what = "all" unless(defined($what));

if(not defined($input_file_name)){
	print "-i flag is mandatory.\n";
	print "USAGE: pull.pl [-o /output/dir/name -n process_name] -i /path/to/input/file\n";
	exit(1);
}

unless(-e $input_file_name){
	print "$input_file_name is not a valid path\n";
	exit;
}

if(not defined($output_dir)){
	$output_dir = cwd();
}
unless(-d $output_dir){
	system("mkdir -p $output_dir");
}
print "Logs are in $output_dir\n";



my %process;
if(defined($benchlist)){
	open PL,"$benchlist" or die "could not open $benchlist\n";
	while(my $l = <PL>){
		chomp($l);
		if(not defined($process{$l})){
			my $bin = benchmarks::get_binary_name($l);
			$process{$bin} = $l;
		}
	}
	close(PL);
}

if(defined($bench)){
	my $bin = benchmarks::get_binary_name($bench);
	$process{$bin} = $bench;
}

if((not defined($bench)) and (not defined($benchlist))){
	$do_all = 1;
}

# Get pids from file
my %pids;
open INF, "grep -P \"^p,\" $input_file_name |";
while(my $line = <INF>){
	chomp($line);
	if($line =~ /^p,(\d+),(.+)$/){
		my $p = $1;
		my $n = $2;
		# If we care about this process.
		if($do_all == 1){
			$pids{$p} = $n;
		} elsif(defined($process{$n})){
			$pids{$p} = $process{$n};
		}
	}
}
close(INF);

# All, or sch
if($what eq "sch" or $what eq "all"){
	foreach my $pid (keys %pids){
		open IN, "grep -P \"^s,\\d+,$pid,\" $input_file_name |";
		my $file_name = "$output_dir/$pids{$pid}.sch";
		my $i = 1;
		while(-e $file_name){
			$file_name = "$output_dir/$pids{$pid}_$i.sch";
			$i++;
		}
		open OUT,"+>$file_name";
		my $tot_inst = 0.0;
		my $tot_refcy = 0.0;
		while(my $l = <IN>){
                        #       Interval,pid,cpu,insts,ipc,req,giv,prev_state,cy
			if($l =~ /s,(\d+),$pid,(\d+),(\d+),(\d\.\d+),(\d),(\d),(\d),(\d+)$/){
				my $interval = $1;
				my $cpu = $2;
				my $insts = $3;
				my $ipc = $4;
				my $req = $5;
				my $giv = $6;
				my $state = $7;
				my $cy = $8;
				$tot_inst += 1.0 * ($insts);
				$tot_refcy += 1.0 * ($cy);
				print OUT "$tot_inst $tot_refcy $interval $insts $cy $cpu $ipc $state $req $giv\n";
			} else {
				print "This should never happen\n";
			}
		}
		close(IN);
		close(OUT);
	}
}


if($what eq "mut" or $what eq "all"){
	open IN, "grep -P \"^m,\" $input_file_name |";
	open OUTG, "+>$output_dir/MUT_GIV";
	open OUTR, "+>$output_dir/MUT_REQ";
	while(my $line = <IN>){
		chomp($line);
		         #     interval req cpu giv cpus
		if($line =~ /^m,(\d+),r,(.+),g,(.+)$/){
			my $interval = $1;
			my $req_str = $2;
			my $giv_str = $3;
			my $req = join(' ',split(/,/,$req_str));
			my $giv = join(' ',split(/,/,$giv_str));
			print OUTR "$interval $req\n";
			print OUTG "$interval $giv\n";
		}
	}
	close(IN);
	close(OUTG);
	close(OUTR);
}

if($what eq "st" or $what eq "all"){
	my %cpu_time;
	open IN, "grep -P \"^t,\" $input_file_name |";
	my $cpu_fh = open_cpus($total_online_cpus,$output_dir);
	while(my $line = <IN>){
		chomp($line);
		  #          
		if($line =~ /t,(\d+),(\d),(\d+)$/){
			my $cpu = $1;
			my $state = $2;
			my $time = $3;
			print_to_cpu($cpu_fh,$cpu,"$state $time\n");
		} else {
			print "Something is wrong\n";
		}
	}
	close(IN);
	close_cpus($cpu_fh);
}

sub open_cpus{
	my $nr_cpus = shift;
	my $path = shift;
	if(not defined($path)){
		$path = ".";
	}
	my @cpu_fh;
	if(not defined($nr_cpus)){
		return;
	}
	for(my $i = 0; $i < $nr_cpus; $i++){
		local *FILE;
		open FILE, "+>$path/CPUST_$i" or die "Could not create $path/CPUST_$i\n";
		$cpu_fh[$i] = *FILE;
	}
	return \@cpu_fh;
}

sub print_to_cpu{
	my $ref_cpufh = shift;
	my $cpu = shift;
	my $string = shift;
	my @cpufh = @$ref_cpufh;
	my $fh = $cpufh[$cpu];

	if(not defined($fh)){
		print "Error, $cpu invalid, opening:\n";
		local *FILE;
		open FILE, "+>CPUST_$cpu" or die "Could not open CPUST_$cpu\n";
		$ref_cpufh->[$cpu] = *FILE;
		$fh = $ref_cpufh->[$cpu];
	}
	print $fh "$string";
}

sub close_cpus{
	my $ref_cpufh = shift;
	my @cpufh = @$ref_cpufh;

	foreach my $fh (@cpufh){
		if(defined($fh)){
			close($fh);
		}
	}
}

