#!/usr/bin/perl
#*************************************************************************
# Copyright 2009 Dan Connors                                             *
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

my $num = 8;
$num = $ARGV[0] if($#ARGV >= 0);

my @existing = ([4,0,0,0,0], [0,4,0,0,0], [0,0,4,0,0], [0,0,0,4,0], [0,0,0,0,4], [1,0,0,0,3], [2,0,0,0,2], [3,0,0,0,1]);

for(my $i=0;$i<$num;$i++){
	my $largest_distance = 10 ** 9;
	my $largest = [0,0,0,0,0];
	my $at_least = 0;
	for(my $a = 0;$a < 5; $a++){
		for(my $b = 0; $b < 5; $b++){
			for(my $c = 0; $c < 5; $c++){
				for(my $d = 0;$d < 5; $d++){
					for(my $e = 0; $e < 5; $e++){
						next if(($a + $b + $c + $d + $e) != 4);
						next if(is_present([$a,$b,$c,$d,$e]));
						my $distance = distance([$a,$b,$c,$d,$e]);
						if($distance < $largest_distance){
							$largest_distance = $distance;
							$largest = [$a,$b,$c,$d,$e];
							$at_least = 1;
						}
					}
				}
			}
		}
	}
	last if($at_least == 0 );
	push @existing, $largest;
	print_node($largest,$largest_distance);

}

sub is_present
{
	my $arr = shift;
	foreach my $ref (@existing){
		my $equal = 1;
		for(my $i=0;$i<5;$i++){
			if($ref->[$i] != $arr->[$i]){
				$equal = 0;
				last;
			}
		}
		if($equal == 1){
			return 1;
		}
	}
	return 0;
}

sub distance
{
	my $arr = shift;
	my $total_squared_distance = 0;
	foreach my $ref  (@existing){
		$total_squared_distance += 1.0 / node_distance_sq($ref, $arr);
	}
	return $total_squared_distance;
}

sub node_distance_sq
{
	my $node1 = shift;
	my $node2 = shift;
	my $sq_distance = 0;
	for(my $i=0;$i<5;$i++){
		$sq_distance += ($node1->[$i] - $node2->[$i]) ** 2;
	}
	return $sq_distance;
}

sub print_node
{
	my $node = shift;
	my $dist = shift;
	print "[$node->[0]";
	for(my $i=1;$i<5;$i++){
		print ",$node->[$i]";
	}
	print "] -> $dist\n";
}

