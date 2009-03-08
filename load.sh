#!/bin/bash
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

function remove {
	MOD=$1
	if [ `lsmod | grep $MOD | wc -l` != "0" ]; then
		echo "${MOD} exists, removing it";
		rmmod $MOD
	fi
}

function insert {
	MOD=$1
	if [ `lsmod | grep $MOD | wc -l` = "0" ]; then
		echo "$MOD does not exist, inserting it"
		modprobe $MOD
	fi
}

function load {
	MOD=$1
	if [ -f $SEEKER_HOME/Build/$MOD.ko ]; then
		echo "Trying to load $MOD";
	else
		echo "Please build seeker (make) before trying to load the modules";
		exit;
	fi
	remove $MOD

	if [ "${MOD}" = "seeker_cpufreq" ]; then
		insert "powernow_k8"
	fi

	if [ "${MOD}" = "seeker_scheduler" ]; then
		insmod $SEEKER_HOME/Build/$MOD.ko $2 $3 $4 $5 $6;
	else
		insmod $SEEKER_HOME/Build/$MOD.ko;
	fi

	if [ "${MOD}" = "seeker_cpufreq" ]; then
		$SEEKER_HOME/seeker_cpufreq.pl start
	fi
}

load "pmu"
load "seeker_cpufreq"
load "seeker_scheduler" $@

echo Done!

