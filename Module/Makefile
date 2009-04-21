

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

include $(SEEKER_HOME)/Makefile.defaults

# All the kernel modules 
obj-m += seeker_scheduler.o           	\
	 pmu.o  	       		\
	 fpmu.o 	       		\
	 seeker_cpufreq.o  		\
	 test_scpufreq.o

# The seeker cpufreq governor and its test module.
test_scpufreq-objs += seeker-cpufreq/test.o
seeker_cpufreq-objs += seeker-cpufreq/main.o

# Performance Monitoring counters.
pmu-objs += pmu/main.o \
	    pmu/intr.o \
	    pmu/msr.o

# Fixed Performance Monitoring counters.
fpmu-objs += fpmu/main.o \
	     fpmu/intr.o \
	     fpmu/msr.o

# Seeker scheduler
seeker_scheduler-objs += seeker-scheduler/main.o         \
		       seeker-scheduler/assigncpu.o      \
		       seeker-scheduler/mutate.o         \
		       seeker-scheduler/state.o          \
		       seeker-scheduler/hwcounters.o     \
		       seeker-scheduler/stats.o	         \
		       seeker-scheduler/tsc_intf.o       \
		       seeker-scheduler/other_mutators.o \
		       seeker-scheduler/nrtasks.o        \
		       seeker-scheduler/debug.o

KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
INCLUDE_DIRS := -I$(PWD)/include

MOD_FLAGS := -DMODULE -m$(BITS) -DARCH_$(ARCA) $(INCLUDE_DIRS) -Wall -O2
MOD_DEBUG_FLAGS := -DMODULE -m$(BITS) -DARCH_$(ARCA) $(INCLUDE_DIRS) -Wall -O0 -DDEBUG -g
ifdef HIST_BASED_SCHEDULING
MOD_FLAGS += -DHIST_BASED_SCHEDULING
MOD_DEBUG_FLAGS += -DHIST_BASED_SCHEDULING
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS_MODULE="$(MOD_FLAGS)" modules
	$(shell if ! test -d ../Build; then mkdir ../Build; fi )
	cp `find . | grep ".ko"` ../Build/
debug:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS_MODULE="$(MOD_DEBUG_FLAGS)" modules
	$(shell if ! test -d ../Build; then mkdir ../Build; fi )
	cp `find . | grep ".ko"` ../Build/

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers modules.order Module.markers
	rm -f ../Build/*.ko
	rm -f ../Build/.*.ko*
	
