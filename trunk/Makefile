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
#   2.  make -f Makefile.new IMPLEMENTATION=<implementation> <target>
#
# Example:
#   make -f Makefile.new IMPLEMENTATION=serviceguard AIS-clm-B.01.01/clm_driver

TIMEOUT_CMD = t0

top_builddir = .

LDFLAGS := -ldl -lpthread -lglib-2.0

VERBOSE = 0

include implementation/$(IMPLEMENTATION)/vars.mk

AIS_VERSION = B.01.01
CLM_DIR = AIS-clm-$(AIS_VERSION)
LCK_DIR = AIS-lck-$(AIS_VERSION)

all: build-tests

build-tests: clean $(TIMEOUT_CMD) $(CLM_DIR)/clm_driver $(LCK_DIR)/lck_driver

tests-pretty:
	$(MAKE) all | column -t -s:

CFLAGS = -g -Wall -Werror -D__AIS_B_TEST__ 

LIBS = -L/usr/local/glib-2.6.3/lib $(IMPLEMENTATION_LIBS)

SHARED_INCLUDE = -I/usr/local/glib-2.6.3/include/glib-2.0 -I/usr/local/glib-2.6.3/lib/glib-2.0/include -Iinclude 

CLM_INCLUDE = $(SHARED_INCLUDE) -I$(CLM_DIR)/include
LCK_INCLUDE = $(SHARED_INCLUDE) -I$(LCK_DIR)/include

driver/driver_utils.o: driver/driver_utils.c
	$(CC) $(CFLAGS) $(LIBS) $(CLM_INCLUDE) -c $< -o $@

driver/saf_driver: driver/driver_main.c driver/driver_utils.o
	$(CC) $(CFLAGS) $(LIBS) $(CLM_INCLUDE) $^ -o $@ $(LDFLAGS) 

$(CLM_DIR)/driver/clm_driver.o: $(CLM_DIR)/driver/clm_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(CLM_INCLUDE) -c $< -o $@

$(CLM_DIR)/driver/clm_driver.so: $(CLM_DIR)/driver/clm_driver.o \
                                 driver/driver_utils.o
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaClm 

$(LCK_DIR)/driver/lck_driver.o: $(LCK_DIR)/driver/lck_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(LCK_INCLUDE) -c $< -o $@

$(LCK_DIR)/driver/lck_driver.so: $(LCK_DIR)/driver/lck_driver.o \
                                 driver/driver_utils.o
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaLck

$(TIMEOUT_CMD):$(TIMEOUT_CMD).c
	@echo Building timeout helper files; \
	$(CC) -O2 -o $@ $< ; \

clean:
	@rm -f $(TIMEOUT_CMD)
	@rm -f driver/saf_driver
	@find . -name "*.o" -print | xargs rm -f
	@find . -name "*.so" -print | xargs rm -f
