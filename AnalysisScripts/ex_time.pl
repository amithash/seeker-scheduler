#!/usr/bin/perl

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

