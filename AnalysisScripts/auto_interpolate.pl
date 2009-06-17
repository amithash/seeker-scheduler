#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;

my $in_dir;
my $out_dir;
my $interval;
my $help;

GetOptions('in=s' => \$in_dir,
	   'out=s' => \$out_dir,
   	   'i|interval=i' => \$interval,
           'h|help' => \$help);
if(defined($help)){
	usage();
}
if((not defined($in_dir)) || (not defined($out_dir))){
	print "--in and --out are mandatory\n";
	usage();
}
if(not defined($interval)){
	$interval = 100;
}

if(not defined($ENV{SEEKER_HOME})){
	print "SEEKER_HOME is not defined!\n";
	exit;
}
my $interp = "$ENV{SEEKER_HOME}/Scripts/interp";
unless (-e $interp){
	print "Please compile seeker! Could not find $interp\n";
	exit;
}

unless(-d $in_dir){
	print "$in_dir does not exist!\n";
	usage();
}
unless(-d $out_dir){
	system("mkdir $out_dir");
}
my @entries = <$in_dir/*.sch.gz>;

my $tmp_dir = "./auto_interp_temp";
system("mkdir $tmp_dir");

foreach my $ent (@entries){
	if($ent =~ /\/([A-Za-z\d]+)\.sch\.gz$/){
		my $name = $1;
		print "working on $name\n";
		system("cat $ent | gunzip > $tmp_dir/$name.sch");
		system("$interp $tmp_dir/$name.sch $out_dir/$name.ich $interval");
	} else {
		print "bad $ent\n";
	}
}
system("rm -r $tmp_dir");






sub usage
{
	print "
	$0 --in /path/to/input/dir --out /path/to/out/dir [--interval <interval>]
	";
	exit;
}
