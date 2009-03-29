
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
ifdef CPUS
EXTRA_ARGS+=CPUS=$(CPUS)
endif
ifdef ARCA
EXTRA_ARGS+=ARCA=$(ARCA)
endif
ifdef HIST_BASED_SCHEDULING
EXTRA_ARGS+=HIST_BASED_SCHEDULING=1
endif

all:
	+make -C Module $(EXTRA_ARGS)
	+make -C lib $(EXTRA_ARGS)
	+make -C Scripts $(EXTRA_ARGS)
	+make -C SyntheticBenchmarks
	+make -C utils
	dot -Tpng design.dot -o design.png
tags:
	ctags -R -u
	cp linux_2.6.28_tags.gz linux.gz
	gunzip linux.gz
	cat linux >> tags
	rm linux

debug:
	+make -C Module debug $(EXTRA_ARGS)
	+make -C lib debug $(EXTRA_ARGS)
	+make -C Scripts debug $(EXTRA_ARGS)
	+make -C SyntheticBenchmarks debug
	+make -C utils debug
	dot -Tpng design.dot -o design.png
clean:
	+make -C Module clean
	+make -C lib clean
	+make -C Scripts clean
	+make -C SyntheticBenchmarks clean
	+make -C utils clean
	-rm design.png

