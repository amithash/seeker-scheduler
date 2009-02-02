#!/usr/bin/perl

use strict;
use warnings;

my $infile = $ARGV[0];
my $outfile = $ARGV[1];

open IN, "$infile" or die "Could not open $infile\n";
open OUT,"+>$outfile" or die "Could not open $outfile\n";
my @cont = <IN>;
close(IN);
chomp($cont[0]);
my @temp = split(/,/,$cont[0]);
my $start = int($temp[0]);
my $last = $start;
my $count = 1;
my $sum = $temp[1] * 1.0;
shift @cont;

foreach my $line (@cont){
	chomp($line);
	my @t = split(/,/,$line);
	my $time = int($t[0]);
	my $pow = $t[1] + 0.0;
	if($time == $last){
		$sum = $sum + $pow;
		$count = $count + 1;
	} else {
		my $p = $sum / ($count * 1.0);
		my $tt = $last - $start;
		print OUT "$tt $p\n";
		$last = $time;
		$sum = $pow;
		$count = 1;
	}
}
my $p = $sum / ($count * 1.0);
my $tt = $last - $start;
print OUT "$tt $p\n";
close(OUT);


