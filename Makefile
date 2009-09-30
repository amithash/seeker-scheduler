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

ifdef CPUS
EXTRA_ARGS+=CPUS=$(CPUS)
endif
ifdef ARCA
EXTRA_ARGS+=ARCA=$(ARCA)
endif
ifdef SCHED_DEBUG
EXTRA_ARGS+=SCHED_DEBUG=1
endif

all:
	+make -C Module $(EXTRA_ARGS)
	+make -C lib $(EXTRA_ARGS)
	+make -C Scripts $(EXTRA_ARGS)
	+make -C SyntheticBenchmarks
	+make -C utils
debug:
	+make -C Module debug $(EXTRA_ARGS)
	+make -C lib debug $(EXTRA_ARGS)
	+make -C Scripts debug $(EXTRA_ARGS)
	+make -C SyntheticBenchmarks debug
	+make -C utils debug
clean:
	+make -C Module clean
	+make -C lib clean
	+make -C Scripts clean
	+make -C SyntheticBenchmarks clean
	+make -C utils clean

