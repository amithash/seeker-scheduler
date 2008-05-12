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
use Getopt::Long;
use Cwd;

my $time = join('',localtime(time));
my $tempdir = cwd() . "/$time";
my $input;
my $output;
my $delete = 0;

GetOptions ('input:s'  => \$input,
            'output:s' => \$output,
	    'delete'   => \$delete);

die "Please set the enviornment variable SEEKER_HOME to the path \nwhere seeker exists if you want to use this script.\n" unless (defined $ENV{SEEKER_HOME});
die "Usage: auto_decode.pl -i /path/to/dir/with/bin/logs -o /path/to/dir/with/output/logs [-delete]\n" unless (defined($input) and defined($output));
die "Please compile seeker before using this script\n" unless (-e "$ENV{SEEKER_HOME}/Scripts/decodelog");
die "$input does not exist!\n" unless (-d "$input");
my $seeker = "$ENV{SEEKER_HOME}";

open LS, "ls $input |";
my @files = <LS>;

# Create a temporary working directory.
system("mkdir $tempdir");
system("mkdir $output") unless(-d "$output");

foreach my $entry (@files){
	chomp($entry);
	print "decoding: $entry\n";
	system("$seeker/Scripts/decodelog < $input/$entry > $tempdir/$entry.txt");
	system("mkdir $output/$entry") unless(-d "$output/$entry");
	system("$seeker/Scripts/pull.pl -i $tempdir/$entry.txt -o $output/$entry/");
}

print "Deleting $tempdir\n" unless ($delete == 0);
system("rm -rf $tempdir") unless ($delete == 0);
print "Temp log files in $tempdir\n" unless($delete == 1);
print "Output Logs in dir $output\n";

