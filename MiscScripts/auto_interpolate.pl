#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: auto_interpolate.pl
 # DESCRIPTION: This file takes in a dir full of log files, passes them one
 # by one into Scripts/interp and generates the output in another provided
 # dir.
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Amithash Prasad                                              *
 #                                                                             *
 # This file is part of Seeker                                                 *
 #                                                                             *
 # Seeker is free software: you can redistribute it and/or modify it under the *
 # terms of the GNU General Public License as published by the Free Software   *
 # Foundation, either version 3 of the License, or (at your option) any later  *
 # version.                                                                    *
 #                                                                             *
 # This program is distributed in the hope that it will be useful, but WITHOUT *
 # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 # FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 # for more details.                                                           *
 #                                                                             *
 # You should have received a copy of the GNU General Public License along     *
 # with this program. If not, see <http://www.gnu.org/licenses/>.              *
 #*****************************************************************************/

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
		my $ind = 0;
		my $outf = "$out_dir/$name.ich";
		if(-e $outf){
			$outf = "$out_dir/$name.$ind.ich";
			while(-e $outf){
				$ind++;
				$outf = "$out_dir/$name.$ind.ich";
			}
		}
		system("$interp $tmp_dir/$name.sch $outf $interval");
		system("rm $tmp_dir/$name.sch");
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
