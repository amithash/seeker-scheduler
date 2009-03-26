#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $cur = cwd();



my %grouph = (
	"High" => "HighIPC0",
	"Low" => "LowIPC1",
	"Low-High" => "Low-HighIPC0",
	"PLow-Low" => "PhaseLow-Low",
	"PLow-High" => "PhaseLow-High",
	"PHigh-PLow" => "PhaseHigh-PhaseLow"
);

my @files = split(/\n/, join("", `ls $cur/raw_*.txt`));

if($#ARGV == 0 and $ARGV[0] eq "-d"){
	foreach my $f (@files){
		if($f =~ /raw_(\d+)_(\d+)_(.+)\.txt$/){
			my $interval = $1;
			my $delta = $2;
			my $group = $3;
			print "Doing $interval, $delta, $group = $grouph{$group}\n";
			system("rm -r log_$interval\_$delta\_$group");
			system("$ENV{SEEKER_HOME}/Scripts/pull.pl --input $f --output $cur/log_$interval\_$delta\_$group --benchlist $ENV{SEEKER_HOME}/AnalysisScripts/group_4/$grouph{$group}");
		}
	}
	exit;
}

foreach my $f (@files){
	if($f =~ /raw_(\d_\d)_(.+)\.txt$/){
		my $level = $1;
		my $group = $2;
		print "Doing $level, $group = $grouph{$group}\n";
		system("rm -r log_$level\_$group");
		system("$ENV{SEEKER_HOME}/Scripts/pull.pl --input $f --output $cur/log_$level\_$group --benchlist $ENV{SEEKER_HOME}/AnalysisScripts/group_4/$grouph{$group}");
	}
}
	
