#!/usr/bin/perl


use strict;
use warnings;

unless(defined($ARGV[0])){
	print "Need input file\n";
	exit;
}
my $inf = $ARGV[0];

open IN, "cat $inf | gunzip |" or die "Could not open $inf!\n";
while(my $line = <IN>){
	chomp($line);
	my @tmp = split(/\s/, $line);
	my $inst = $tmp[3];
	my $rfcy = $tmp[4];
	my $reipc = $tmp[6];
	my $st = $tmp[7];
	if($rfcy > (50 * 10**9)){
		next;
	}
	my $rfipc = $inst / $rfcy;
	printf("%d %.3f %.3f\n", $st, $rfipc, $reipc);
}

