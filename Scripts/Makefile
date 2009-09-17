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

include $(SEEKER_HOME)/Makefile.defaults

ifeq ($(MAKECMDGOALS),debug)
CFLAGS = -O0 -DDEBUG
else
CFLAGS = -O3
endif

CC = gcc
CXX = g++
SEEKER_INCLUDE := -I$(shell pwd)/../Module/include
STATE_LIB := $(shell pwd)/../lib


all: decodelog seekerlogd test_recommendations interp maxmin smooth

debug: decodelog seekerlogd test_recommendations interp maxmin smooth

decodelog: decodelog.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) $(SEEKER_INCLUDE) -DNR_CPUS=$(CPUS) -DARCH_$(ARCA) -o $@
seekerlogd: seekerlogd.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) $(SEEKER_INCLUDE) -DNR_CPUS=$(CPUS) -DARCH_$(ARCA) -o $@
test_recommendations: test_recommendations.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) -lstate_recommendations -L$(STATE_LIB) -o $@
interp: interp.cpp
	$(CXX) $(CFLAGS) $? $(LDFLAGS) -o $@
maxmin: maxmin.cpp
	$(CXX) $(CFLAGS) $? $(LDFLAGS) -o $@
smooth: smooth.cpp
	$(CXX) $(CFLAGS) $? $(LDFLAGS) -o $@

clean:
	rm -f seekerlogd interp maxmin smooth decodelog test_recommendations

