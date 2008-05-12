#!/bin/sh
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

for MOD in "seeker_sampler" "c2dpmu" "c2dfpmu" "c2dtsc" "c2dtherm"; do
	lsmod | grep $MOD &> /dev/null
	if [ "$?" != "0" ]; then
		echo "$MOD does not seem to be loaded."
	else
		rmmod $MOD;
		sleep 1;
		lsmod | grep $MOD &> /dev/null
		if [ "$?" != "0" ]; then
			echo "Successfully unloaded $MOD."
		else
			echo "Problems encountered in unloading $MOD"
		fi
	fi
done



echo Done!


