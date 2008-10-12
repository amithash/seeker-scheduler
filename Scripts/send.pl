#!/usr/bin/perl
#*************************************************************************
# Copyright 2008 Amithash Prasad                                         *
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
use Config;

defined $Config{sig_name} or die "No sigs?";
my $i = 0;     # Config prepends fake 0 signal called "ZERO".
my %signo;
foreach my $name (split(' ', $Config{sig_name})) {
	$signo{$name} = $i;
	$i++;
}

my $sigusr1 = $signo{USR1} + 0;
my $sigterm = $signo{TERM} + 0;

my $terminate = (defined($ARGV[0]) and $ARGV[0] eq "-t") ? 1 : 0;

open PS,"ps -e | grep \"seekerd\" |";
my @list = split(/\n/,join("",<PS>));
my @pid = (0,0);
$i = 0;
foreach my $entry (@list){
	my @ps = split(/\s+/,<PS>);
	if(defined($ps[0])){
		foreach my $element (@ps){
			if($element =~ /(\d+)/){
				$pid[$i] = $1 + 0;
				$i++;
				last;
			}
		}
	}
	else{
		print "seekerd does not seem to be executing!\n";
		exit;
	}
}

if($terminate == 0){
	print "Requesting seekerd to change logs\n";
	print "Sending Signal:$sigusr1 to pid $pid[1] and $pid[0]\n";
	kill $sigusr1, $pid[0];
	kill $sigusr1, $pid[1];
}
else{
	print "Terminating seekerd\n";
	kill $sigterm, $pid[0];
	kill $sigterm, $pid[1];
}

