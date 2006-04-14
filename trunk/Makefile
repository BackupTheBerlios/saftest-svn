# Copyright (c) 2005, safest Project.
# http://sourceforge.net/projects/saftest
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA 02111-1307 USA.
#
# Authors:
#   Tindel, Chad <chad.tindel@hp.com>
#
# Usage:
#   1.  Fill out the implementation specific vars.mk
#   2.  make IMPLEMENTATION=<implementation> SAFLIBPATH=<SAF Library Path>  <LIB>=1 <LIB>=1 <target>
#
# Examples:
#   make IMPLEMENTATION=serviceguard CLM=1 LCK=1 saftest 
#   make SAFLIBPATH=/usr/local/cmcluster/lib IMPLEMENTATION=serviceguard CLM=1 LCK=1 saftest 
#

VERBOSE = 0
CLM = 0
LCK = 0
MSG = 0

.PHONY: saftest

# PLATFORM to hpux|linux and default SAF library path
platform := $(shell uname -s)
PLATFORM=undefined
ifeq ($(platform), HP-UX)
   PLATFORM=hpux
   ifeq ($(SAFLIBPATH), )
       SAFLIBPATH=/usr/lib
    endif
endif
ifeq ($(platform), Linux)
   PLATFORM=linux
   ifeq ($(SAFLIBPATH), )
       SAFLIBPATH=/usr/local/cmcluster/lib
    endif
endif

PLATFORM_INCLUDES=

include implementation/$(IMPLEMENTATION)/vars.mk
include build/platform_vars_$(PLATFORM).mk
include build/vars.mk
include build/platform_targets_common.mk
include build/platform_targets_$(PLATFORM).mk

# This will get appended to in the individual specification vars.mk files
test_libs=

ifeq ($(CLM), 1)
include cases/clm/build/vars.mk
endif

ifeq ($(LCK), 1)
include cases/lck/build/vars.mk
endif

#ifeq ($(MSG), 1)
#include cases/msg/vars.mk
#endif

saftest: $(FINAL_OBJDIR)/dir_exists $(FINAL_OBJDIR)/saf_driver $(test_libs)

clean:
	rm -rf $(OBJDIR)
