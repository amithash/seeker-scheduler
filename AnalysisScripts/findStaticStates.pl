#!/usr/bin/perl
 #*****************************************************************************\
 # FILE: findStaticStates.pl
 # DESCRIPTION: Generate a list of static processor layouts with 5 performance
 # states. 
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Dan Connors                                                  *
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

use Getopt::Long;
use File::Find;
use Cwd;
use Time::HiRes qw( gettimeofday tv_interval);

$states = 5;

$existing[0][0] = 2;
$existing[0][1] = 0;
$existing[0][2] = 0;
$existing[0][3] = 0;
$existing[0][4] = 2;


$existing[1][0] = 0;
$existing[1][1] = 0;
$existing[1][2] = 0;
$existing[1][3] = 0;
$existing[1][4] = 4;

$existing[2][0] = 4;
$existing[2][1] = 0;
$existing[2][2] = 0;
$existing[2][3] = 0;
$existing[2][4] = 0;

$existing[3][0] = 1;
$existing[3][1] = 0;
$existing[3][2] = 0;
$existing[3][3] = 0;
$existing[3][4] = 3;

$existing[4][0] = 3;
$existing[4][1] = 0;
$existing[4][2] = 0;
$existing[4][3] = 0;
$existing[4][4] = 1;

$existing[5][0] = 0;
$existing[5][1] = 4;
$existing[5][2] = 0;
$existing[5][3] = 0;
$existing[5][4] = 0;

$existing[6][0] = 0;
$existing[6][1] = 0;
$existing[6][2] = 0;
$existing[6][3] = 4;
$existing[6][4] = 0;

$existing[7][0] = 0;
$existing[7][1] = 0;
$existing[7][2] = 4;
$existing[7][3] = 0;
$existing[7][4] = 0;

#@existing[1] = ([4],[0],[0],[0],[0]);
#@existing[2] = ([0],[0],[0],[0],[4]);
#@existing[3] = ([1],[0],[0],[0],[3]);
#@existing[4] = ([3],[0],[0],[0],[1]);
#@existing[5] = ([0],[4],[0],[0],[0]);
#@existing[6] = ([0],[0],[4],[0],[0]);
#@existing[7] = ([0],[0],[0],[4],[0]);

$index = 0;

$already = 8;


for ($i=0; $i < $states; $i++) {
   for ($j=0; $j < $states; $j++) {
      for ($k=0; $k < $states; $k++) {
         for ($x=0; $x < $states; $x++) {
            for ($y=0; $y < $states; $y++) {

		 if ($i + $j + $k + $x + $y != 4) {
			next;
		 }

		 $found = 0;
		 for ($e=0; $e < $already; $e++) {
			if ($existing[$e][0] == $i  && 
			   $existing[$e][1] == $j  && 
			   $existing[$e][2] == $k  && 
			   $existing[$e][3] == $x  && 
			   $existing[$e][4] == $y ) {
			   $found = 1;
			}
		 }

		if ($found == 1) {
		   next;
		}

		# check to see if this is one of the existing

			#print "[$i, $j, $k, $x, $y]\n";
                          $potential[$index][0] = $i;
                          $potential[$index][1] = $j;
                          $potential[$index][2] = $k;
                          $potential[$index][3] = $x;
                          $potential[$index][4] = $y;
			  $index = $index + 1;
            }   
	 }
       }
   }
}


  #for ($i=0; $i < $index; $i++) {
  #printf "[%d, %d, %d, %d, %d]\n", $potential[$i][0], $potential[$i][1], $potential[$i][2], $potential[$i][3], $potential[$i][4];
  #}


  for ($p=0; $p < $index; $p++) {
     for ($e=0; $e < $already; $e++) {
      
	# print " comparing $p and $e\n";
	$dist = 0; 
        for ($d=0; $d < 5; $d++) {
	#	printf "p %d    e   %d " , $potential[$p][$d], $existing[$e][$d];
		$dist += ($potential[$p][$d] - $existing[$e][$d]) * ($potential[$p][$d] - $existing[$e][$d]);
	# print "$dist ";
        }
	# print " \n";
	$distance[$p] += sqrt($dist);
     }
  }

  for ($i=0; $i < $index; $i++) {
    #printf "%f [%d, %d, %d, %d, %d] \n", $distance[$i],  $potential[$i][0], $potential[$i][1], $potential[$i][2], $potential[$i][3], $potential[$i][4];

     $bestvector{$i} = $distance[$i];
  }

 # Sort the results
 @sorted_keys = sort {    
    $bestvector{$b} <=> $bestvector{$a}
  } keys %bestvector;  


  foreach $i (@sorted_keys) {
    printf "[%d, %d, %d, %d, %d]  %f\n", $potential[$i][0], $potential[$i][1], $potential[$i][2], $potential[$i][3], $potential[$i][4], $distance[$i];
  }

