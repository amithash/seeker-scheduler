#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: genLOG.pl
 # DESCRIPTION: 
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Dan Connors                                                  *
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

#use warnings;
use Getopt::Long;
use File::Find;
use Cwd;
use Time::HiRes qw( gettimeofday tv_interval);

############################################################################## 
# GLOBALS 							             #
##############################################################################
my $currentdir = cwd();
my $org_path = cwd();
my $help;
my $file;

my $print_bench;
my $print_cpu;
my $output = "LOG";

###############################################################################
# SUBROUTINES                                                                 #
###############################################################################
sub help {
    print ("profileDVFS.pl -targetdir (dir) \n");
   exit 1;
}

sub read_file
{
    my @ret;
    open(FH, "< $_[0]") or die("Unable to open file $_[0]\n");
    @ret = map { chomp; $_ } <FH>;
    close(FH);
    return @ret;
}



###############################################################################
GetOptions (
'file:s' => \$file,
'cpu' => \$print_cpu,
'bench' => \$print_bench,
'stats' => \$print_stats,
'output:s' => \$output,
	    'help' => \$help);

# Ask for help
if(defined $help){
    help();
}


@files = read_file($ARGV[0]);

system ("rm -r -f $output");

if ($print_cpu) {
   $print_cpu = "-cpu"
}
if ($print_bench) {
   $print_bench = "-bench"
}

foreach $file (@files) {
    print "working on $file \n";
    system ("analyzeDVFS.pl -target $file $print_cpu $print_bench >> $output");
}

