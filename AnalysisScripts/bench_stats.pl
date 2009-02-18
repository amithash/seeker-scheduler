#!/usr/bin/perl

use strict;
use warnings;

my %stats = (	'[0.00,0.25)' => 0,
   		'[0.25,0.50)' => 0,
   		'[0.50,0.75)' => 0,
   		'[0.75,1.00)' => 0,
   		'[1.00,1.25)' => 0,
   		'[1.25,1.50)' => 0,
   		'[1.50,1.75)' => 0,
   		'[1.75,2.00)' => 0,
   		'[2.00,2.25)' => 0,
   		'[2.25,5.00)' => 0,
   		'[5.00,inf)' => 0
	);
my %residency;

if($#ARGV != 0){
	print "USAGE: $0 file";
	exit;
}

my $total = 0.0;

open IN, "$ARGV[0]" or die "Opening $ARGV[0] failed with $!\n";
while(my $line = <IN>){
	chomp($line);
	my @l = split(/,/,$line);
	my $ipc = $l[0];
	my $cy = $l[1];
	my $state = $l[2];
	classify($ipc,$cy * 1.0);
	residency($state,$cy * 1.0);
	$total += ($cy*1.0);
}

foreach my $ipc (keys %stats){
	if($stats{$ipc} == 0.0){
		next;
	}
	my $perc = $stats{$ipc} * 100 / $total;
	my $perc_s = sprintf("%.2f",$perc);
	print "$ipc\t$perc_s\n";
}
print "\n\n";
foreach my $state (sort keys %residency){
	my $cyb = $residency{$state} / (10.0 ** 9);
	my $cyb_s = sprintf("%.4f",$cyb);
	print "$state\t$cyb_s\n";
}

sub residency{
	my $state = shift;
	my $cy = shift;
	if(defined($residency{$state})){
		$residency{$state} += $cy;
	} else {
		$residency{$state} = $cy;
	}
}

sub calassify{
	my $ipc = shift;
	my $cy = shift;

	if($ipc >= 0 and $ipc < 0.25){
		$stats{'[0.00,0.25)'} += $cy;
		return;
	}
	if($ipc >= 0.25 and $ipc < 0.5){
		$stats{'[0.25,0.50)'} += $cy;
		return;
	}
	if($ipc >= 0.5 and $ipc < 0.75){
		$stats{'[0.50,0.75)'} += $cy;
		return;
	}
	if($ipc >= 0.75 and $ipc < 1.0){
		$stats{'[0.75,1.00)'} += $cy;
		return;
	}
	if($ipc >= 1.0 and $ipc < 1.25){
		$stats{'[1.00,1.25)'} += $cy;
		return;
	}
	if($ipc >= 1.25 and $ipc < 1.5){
		$stats{'[1.25,1.50)'} += $cy;
		return;
	}
	if($ipc >= 1.50 and $ipc < 1.75){
		$stats{'[1.50,1.75)'} += $cy;
		return;
	}
	if($ipc >= 1.75 and $ipc < 2.0){
		$stats{'[1.75,2.00)'} += $cy;
		return;
	}
	if($ipc >= 2.0 and $ipc < 2.25){
		$stats{'[2.00,2.25)'} += $cy;
		return;
	}
	if($ipc >= 2.25 and $ipc < 5.0){
		$stats{'[2.25,5.00)'} += $cy;
		return;
	}
	if($ipc >= 5.0){
		$stats{'[5.00,inf)'} += $cy;
		return;
	}
	#should never be executed.
	print "error";
}

