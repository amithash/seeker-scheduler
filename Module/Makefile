

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
#*************************************************************************/
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
endif

ifneq ($(KERNELRELEASE),)


obj-m += seeker_scheduler.o           	\
	 seeker_sampler.o             	\
	 pmu/pmu.o         		\
	 fpmu/fpmu.o        		\
	 tsc/tsc.o         		\
	 therm/therm.o       		\
	 scpufreq/scpufreq.o  		\
	 test_scpufreq.o		\
	 power/power.o

seeker_sampler-objs += seeker-sampler/alloc.o   \
		       seeker-sampler/log.o     \
		       seeker-sampler/probe.o   \
		       seeker-sampler/sample.o  \
		       seeker-sampler/io.o      \
		       seeker-sampler/intr.o    \
		       seeker-sampler/main.o

seeker_scheduler-objs += seeker-scheduler/main.o     \
		       seeker-scheduler/assigncpu.o  \
		       seeker-scheduler/mutate.o     \
		       seeker-scheduler/state.o      \
		       seeker-scheduler/hwcounters.o \
		       seeker-scheduler/debug.o
else
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
INCLUDE_DIRS := -I$(PWD)/include	\
		-I$(PWD)/pmu		\
		-I$(PWD)/fpmu		\
		-I$(PWD)/tsc		\
		-I$(PWD)/therm		\
		-I$(PWD)/scpufreq	\

MOD_FLAGS := -DMODULE -m64 -DARCH_$(ARCA) $(INCLUDE_DIRS) -Wall

# Use -g and -O0 while defineing DEBUG for debug builds, else just use O2
ifdef DEBUG
MOD_FLAGS += -DDEBUG -g -O0
else
MOD_FLAGS += -O0
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS_MODULE="$(MOD_FLAGS)" modules
	$(shell if ! test -d ../Build; then mkdir ../Build; fi )
	cp `find . | grep ".ko"` ../Build/
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers modules.order Module.markers
	rm -f ../Build/*.ko
	rm -f ../Build/.*.ko*
	
endif
