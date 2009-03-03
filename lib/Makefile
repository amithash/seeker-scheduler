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

include $(SEEKER_HOME)/Makefile.defaults

CFLAGS = -O3
CC = gcc
CXX = g++

all:
	${CC} -fPIC -c state_recommendations.c -o state_recommendations.o
	${CC} -shared -Wl,-soname,libstate_recommendations.so -o libstate_recommendations.so state_recommendations.o -lc 

debug:
	${CC} -fPIC -c state_recommendations.c -o state_recommendations.o -DDEBUG
	${CC} -shared -Wl,-soname,libstate_recommendations.so -o libstate_recommendations.so state_recommendations.o -lc 
clean:
	rm -f libstate_recommendations.so state_recommendations.o

