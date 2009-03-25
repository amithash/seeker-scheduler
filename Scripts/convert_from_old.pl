#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $cur = cwd();

my %grouph = (
	"High" => "HighIPC0",
	"Low" => "LowIPC1",
	"Low-High" => "Low-HighIPC0",
	"PLow-Low" => "PhaseLow-LowIPC",
	"Plow-High" => "PhaseLow-High",
	"PHigh-PHigh" => "PhaseHigh-PhaseLow"
);

my @files = split(/\n/, join("", `ls $cur/raw_*.txt`));

foreach my $f (@files){
	if($f =~ /raw_(\d_\d)_(.+)\.txt$/){
		my $level = $1;
		my $group = $2;
		system("rm -r log_$level\_$group");
		system("$ENV{SEEKER_HOME}/Scripts/pull.pl --input $f --output $cur/log_$level\_$group --benchlist $ENV{SEEKER_HOME}/AnalysisScripts/group_4/$grouph{$group}");
	}
}

