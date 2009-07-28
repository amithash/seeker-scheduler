#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $this_dir = cwd();
my @logs = <$this_dir/log_*>;
foreach my $log (@logs){
	open GIV,"cat $log/MUT_GIV.gz | gunzip |" or die "Could not open MUT_GIV.gz in $log\n";
	open REQ,"cat $log/MUT_REQ.gz | gunzip |" or die "Could not open MUT_GIV.gz in $log\n";
	my $count = 0;
	my $dist_sum = 0;
	while(my $giv_l = <GIV>){
		my $req_l = <REQ>;
		my @giv = split(/\s/,$giv_l);
		my @req = split(/\s/,$req_l);
		shift @giv;
		shift @req;
		my $dist = manhattan_distance(\@giv, \@req);
		$dist_sum += $dist;
		$count++;
	}
	close(GIV);
	close(REQ);
	my @tmp = split(/\//,$log);
	my $t = $tmp[$#tmp];
	my @lst = split(/_/,$t);
	my $interval = $lst[1];
	my $delta = $lst[2];
	my $wrk = $lst[3];
	my $avg_err = $dist_sum / $count;
	printf("$wrk $interval $delta %.4f\n",$avg_err);
}
sub manhattan_distance
{
	my $ref_a = shift;
	my $ref_b = shift;
	my @a = @$ref_a;
	my @b = @$ref_b;
	if($#a != $#b){
		print "In equal dimensions!\n";
		return;
	}
	my $dist = 0;
	for(my $i=0; $i<=$#a;$i++){
		$dist += abs($a[$i] - $b[$i]);
	}
	return $dist;
}

