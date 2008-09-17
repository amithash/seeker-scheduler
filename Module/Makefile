

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
ARCA := C2D
endif

ifneq ($(KERNELRELEASE),)


obj-m += seeker-sampler.o pmu.o fpmu.o tsc.o therm.o dvfs.o
seeker-sampler-objs += alloc.o log.o probe.o sample.o io.o intr.o main.o

else
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
ifndef DEBUG
MOD_FLAGS := -DMODULE -O3 -march=nocona -DARCH_$(ARCA)
else
MOD_FLAGS := -DMODULE -O3 -march=nocona -DARCH_$(ARCA) -DDEBUG
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS_MODULE="$(MOD_FLAGS)" modules
	$(shell if ! test -d ../Build; then mkdir ../Build; fi )
	cp *.ko ../Build
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers modules.order Module.markers
	rm -f ../Build/*

endif
