#!/bin/sh
 #*****************************************************************************\
 # FILE: unload.sh
 # DESCRIPTION: Unloads the modules if found to be loaded.
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

function remove {
	MOD=$1
	if [ `lsmod | grep $MOD | wc -l` != "0" ]; then
		echo "${MOD} exists, removing it";
		rmmod $MOD
	fi
}

remove "seeker_scheduler"
remove "pmu"
$SEEKER_HOME/seeker_cpufreq.pl stop
remove "seeker_cpufreq"

echo Done!


