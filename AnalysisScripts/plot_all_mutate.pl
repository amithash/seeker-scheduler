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
use Cwd;

my $pm = "/thesis/ms_thesis-amithash_prasad/scripts/plot_mutate.pl";
my $tmp = "/thesis/tmp/";
my $t = "jpeg";
my $this = cwd();
#chdir("/thesis/ms_thsis-amithash_prasad/scripts");
my @dirs = <$this/log_*>;
my $out = "/thesis/mut_sel";
system("mkdir $out") unless(-d $out);

foreach my $dir (@dirs){
	if($dir =~ /log_(\d+)_(\d+)_([a-zA-Z-]+)$/){
		my $interval = $1;
		my $delta = $2;
		my $wrk = $3;
		print "$interval : $delta : $wrk\n";
		system("$pm -in $dir -out $tmp -type $t");
		system("mkdir $out/$wrk") unless(-d "$out/$wrk");
		system("mv $tmp/giv.$t $out/$wrk/${interval}_${delta}.$t");
		system("rm $tmp/*");
	}
}
#chdir($this);
