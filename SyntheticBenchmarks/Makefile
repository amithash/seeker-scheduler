#*************************************************************************
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

all: synth_bench
debug: synth_bench_debug

CACHE_SIZE=$(shell perl -e 'my $$a = `cat /proc/cpuinfo | grep "cache size"`;$$a =~ /(\d+)/; print $$1')

synth_bench:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DCACHE_SIZE="$(CACHE_SIZE).0"
synth_bench_debug:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DCACHE_SIZE="$(CACHE_SIZE).0" -DDEBUG
clean:
	rm -f synth_bench
