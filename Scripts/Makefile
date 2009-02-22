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
ifndef CPUS
CPUS := $(shell cat /proc/cpuinfo | grep processor | wc -l)
endif
CFLAGS = -O3
CC = gcc
CXX = g++
ifndef ARCA
ARCA := C2D
endif
INCLUDE := -I$(shell pwd)/../Module/include

all:
	${CC} ${CFLAGS} debugd.c -o debugd -DARCH_$(ARCA) $(INCLUDE) -DNR_CPUS=$(CPUS)
	${CC} ${CFLAGS} decodelog.c -o decodelog -DARCH_$(ARCA) $(INCLUDE) -DNR_CPUS=$(CPUS)
	${CXX} ${CFLAGS} interp.C -o interp -g
	${CXX} ${CFLAGS} maxmin.C -o maxmin
	${CXX} ${CFLAGS} smooth.C -o smooth
	${CC} -fPIC -c state_recommendations.c -o state_recommendations.o
	${CC} -shared -Wl,-soname,libstate_recommendations.so -o libstate_recommendations.so state_recommendations.o -lc 
	${CC} test_recommendations.c -o test_recommendations -lstate_recommendations -L.

clean:
	rm -f decodelog interp maxmin smooth debugd test_recommendations libstate_recommendations.so state_recommendations.o

