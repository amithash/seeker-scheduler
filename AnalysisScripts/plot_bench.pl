#!/usr/bin/perl
#*************************************************************************
# Copyright 2009 Amithash Prasad                                         *
#                                                                        *
# This file is part of Seeker                                            *
#                                                                        *
# Seeker is free software: you can redistribute it and/or modify         *
# it under the terms of the GNU General Public License as published by   *
# the Free Software Foundation, either version 3 of the License, or      *
# (at your option) any later version.                                    *
#                                                                        *
# This program is distributed in the hope that it will be useful,        *
# but WITHOUT ANY WARRANTY; without even the implied warranty of         *
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
# GNU General Public License for more details.                           *
#                                                                        *
# You should have received a copy of the GNU General Public License      *
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
#*************************************************************************

use strict;
use warnings;
use Getopt::Long;

my $pl = "/usr/bin/ploticus";
$pl = "./pl" if(`which $pl` !~ /^\//);
print "using: $pl\n";

my $plot_string = "
#proc getdata
file: IN_DATA
delim: space
#endproc

#proc areadef
  rectangle: 1 3.75 6.5 6.5
  xautorange: datafields=2 combomode=normal incmult=2.0 nearest=auto
  yautorange: datafields=7 combomode=normal incmult=2.0  nearest=auto

#proc xaxis
  stubs: inc 
  label: Reference Cycles

#proc yaxis
  stubs: inc 
  stubcull: yes
  label: Instructions Per Real Clock

#proc lineplot 
 xfield: 2
 yfield: 7
 linedetails: width=1 color=rgb(1,0,0)

#proc areadef
  rectangle: 1 1 6.5 3.0
  xautorange: datafields=2 combomode=normal incmult=2.0 nearest=auto
  yautorange: datafields=8 combomode=normal incmult=2.0  nearest=auto

#proc xaxis
  stubs: inc 
  label: Reference Cycles

#proc yaxis
  stubs: inc 
  stubcull: yes
  label: Performance State

#proc lineplot 
 xfield: 2
 yfield: 8
 linedetails: width=1 color=rgb(1,0,0)

";

sub help
{
	my $error = shift;
	if(defined($error)){
		print "$error\n";
	}
	print "Usage: $0 -in <Input Log Dir> -out <Output dir for plots> [-type = <image type>, default: png]\n";
	exit;
}

my $input; 
my $output;
my $type;
my $help;
my $giv_c = 0;
my $req_c = 0;
GetOptions(
	'in=s' => \$input,
	'out=s' => \$output,
	'type=s' => \$type,
	'help' => \$help
);
help() if(defined($help));
help("Error input and output param are mandatory") if(not defined($input) or not defined($output));
$type = "png" if(not defined($type));

unless(-d $output){
	system("mkdir -p $output");
}
help("$input does not exist!") unless(-d $input);

my @in = <$input/*.sch.gz>;

print "Starting\n";
foreach my $file (@in){
	if($file =~ /^(\S+)\/(.+)\.sch\.gz$/){
		my $bench = $2;
		system("gunzip $file");
		my $f = "$input/$bench.sch";
		my $plot = $plot_string;
		$plot =~ s/IN_DATA/$f/;
		print "FILE=$file\tBENCH=$bench\tF=$f\n";
		open PLT, "+>$output/tmp.plot" or die "Could not create $output/tmp.plot\n";
		print PLT "$plot";
		close(PLT);
		system("$pl $output/tmp.plot -maxrows 100000000 -maxfields 10000000 -maxvector 1000000 -o $output/$bench.$type -$type") == 0 or print "FAiled 1\n";
		system("gzip $f");
		system("rm $output/tmp.plot") == 0 or print "Failed\n";
	}
}
