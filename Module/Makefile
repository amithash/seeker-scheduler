

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

# First try and get the architecture...

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
ifeq ($(shell uname -m | grep x86_64 | wc -l), 1)
BITS := 64
else
BITS := 32
endif

# get what mutator should be compiled.
ifdef ALT
MUTATE := mutate_alt$(ALT).o
else
MUTATE := mutate.o
endif

ifneq ($(KERNELRELEASE),)

# All the kernel modules 
obj-m += seeker_scheduler.o           	\
	 seeker_sampler.o             	\
	 pmu.o  	       		\
	 fpmu.o 	       		\
	 tsc.o 	        		\
	 therm.o	       		\
	 seeker_cpufreq.o  		\
	 test_scpufreq.o

# The seeker cpufreq governor and its test module.
test_scpufreq-objs += seeker-cpufreq/test.o
seeker_cpufreq-objs += seeker-cpufreq/main.o

# TSC Module
tsc-objs += tsc/main.o

# Temperature module.
therm-objs += therm/main.o

# Seeker sampler. 
seeker_sampler-objs += seeker-sampler/alloc.o   \
		       seeker-sampler/log.o     \
		       seeker-sampler/probe.o   \
		       seeker-sampler/sample.o  \
		       seeker-sampler/io.o      \
		       seeker-sampler/intr.o    \
		       seeker-sampler/main.o

# Seeker scheduler
seeker_scheduler-objs += seeker-scheduler/main.o     \
		       seeker-scheduler/assigncpu.o  \
		       seeker-scheduler/$(MUTATE)    \
		       seeker-scheduler/state.o      \
		       seeker-scheduler/hwcounters.o \
		       seeker-scheduler/stats.o	     \
		       seeker-scheduler/debug.o

# Performance Monitoring counters.
pmu-objs += pmu/main.o \
	    pmu/intr.o \
	    pmu/msr.o

# Fixed Performance Monitoring counters.
fpmu-objs += fpmu/main.o \
	     fpmu/intr.o \
	     fpmu/msr.o

else
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
INCLUDE_DIRS := -I$(PWD)/include

MOD_FLAGS := -DMODULE -m$(BITS) -DARCH_$(ARCA) $(INCLUDE_DIRS) -Wall

# Use -g and -O0 while defineing DEBUG for debug builds, else just use O2
ifdef DEBUG
MOD_FLAGS += -DDEBUG -g -O0
else
MOD_FLAGS += -O2
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
