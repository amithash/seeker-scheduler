#!/usr/bin/perl

use strict;
use warnings;

if($#ARGV != 0){
	print "USAGE: $0 FILE\n";
	exit;
}
my $ipmi = `which ipmitool`;
my $date = `which date`;
chomp($ipmi);
chomp($date);
if(! -e $ipmi || !-e $date){
	die "ipmitool is not installed\n";
}

my $Voltage = "PS0/V_OUT";
my $Current = "PS0/I_OUT";
my $outfile = $ARGV[0];

my $pid = fork();
if($pid != 0){
	exit;
}

open OUT, "+>$outfile" or die "Could not open or create $outfile\n";
while(1){
	my $current = 0;
	my $voltage = 0;
	my $power = 0;
	open IN, "$ipmi sensor get $Voltage $Current | grep \"Sensor Reading\" |" or warn "Could not execute $ipmi\n";
	my @temp = <IN>;
	close(IN);
	foreach my $entry (@temp){
		if($entry =~ /Sensor Reading\s+:\s+(\d+\.\d+)\s+.+Volts/){
			$voltage = $1 + 0.0;
		} elsif($entry =~ /Sensor Reading\s+:\s+(\d+\.\d+)\s+.+Amps/){
			$current = $1 + 0.0;
		}
	}
	
	if($voltage == 0 or $current == 0){
		print "Something is wrong\n";
		next;
	}
	$power = $voltage * $current;
	my $interval = `/bin/date +"%s"`;
	chomp($interval);
	print OUT "$interval,$power\n";
}
close(OUT);
close(IN);

