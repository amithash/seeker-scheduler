

#*************************************************************************
# Copyright 2008 Amithash Prasad                                         *
# Copyright 2006 Tipp Mosely                                             *
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

# ARCA must be set to the architecture in use.
ARCA := c2d

ifneq ($(KERNELRELEASE),)

obj-m += seeker-sampler.o $(ARCA)/$(ARCA)pmu.o $(ARCA)/$(ARCA)fpmu.o $(ARCA)/$(ARCA)tsc.o $(ARCA)/$(ARCA)therm.o
seeker-sampler-objs += main.o log.o intr.o probe.o alloc.o io.o sample.o

else
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
MOD_FLAGS := -I$(PWD)/$(ARCA) -DMODULE -B$(PWD)/$(ARCA) -O3 -march=nocona

default:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS_MODULE="$(MOD_FLAGS)" modules
	$(shell if ! test -d ../Build; then mkdir ../Build; fi )
	cp *.ko ../Build
	cp $(ARCA)/*.ko ../Build
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers
	rm -f ../Build/*

endif
