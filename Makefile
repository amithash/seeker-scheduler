
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
ifndef ARCA
ARCA := $(shell cat /proc/cpuinfo | grep -i AuthenticAMD | wc -l)
ifneq ($(ARCA), 0)
ARCA := K10
else
ARCA := $(shell cat /proc/cpuinfo | grep -i AuthenticIntel | wc -l)
ifneq ($(ARCA), 0)
ARCA := C2D
else 
ARCA := C2D
endif
endif
#ARCA := C2D
endif

ifndef CPUS
CPUS := $(shell cat /proc/cpuinfo | grep processor | wc -l)
endif

all:
	+make -C Module ARCA=$(ARCA) $(EXTRA_ARGS)
	+make -C Scripts ARCA=$(ARCA) CPUS=$(CPUS) $(EXTRA_ARGS)
	+make -C SyntheticBenchmarks

debug:
	+make -C Module ARCA=$(ARCA) $(EXTRA_ARGS) debug
	+make -C Scripts ARCA=$(ARCA) CPUS=$(CPUS) $(EXTRA_ARGS) debug
	+make -C SyntheticBenchmarks debug
clean:
	+make -C Module clean
	+make -C Scripts clean
	+make -C SyntheticBenchmarks clean

