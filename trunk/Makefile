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
#   make IMPLEMENTATION=serviceguard driver/saf_driver AIS-clm-B.01.01/driver/clm_driver.so

TIMEOUT_CMD = t0

LDFLAGS := -ldl -lpthread -lglib-2.0

VERBOSE = 0

include implementation/$(IMPLEMENTATION)/vars.mk

AIS_VERSION = B.01.01
CLM_DIR = AIS-clm-$(AIS_VERSION)
LCK_DIR = AIS-lck-$(AIS_VERSION)
MSG_DIR = AIS-msg-$(AIS_VERSION)

all: driver/saf_driver libs

libs: $(CLM_DIR)/driver/clm_driver.so \
      $(LCK_DIR)/driver/lck_driver.so \
      $(MSG_DIR)/driver/msg_driver.so

CFLAGS = -g -Wall -Werror -D__AIS_B_TEST__ 

LIBS = -L/usr/local/glib-2.6.3/lib $(IMPLEMENTATION_LIBS)

STANDARD_INCLUDE = -Iinclude/standard

SHARED_INCLUDE = -I/usr/local/glib-2.6.3/include/glib-2.0 -I/usr/local/glib-2.6.3/lib/glib-2.0/include -Iinclude $(STANDARD_INCLUDE)

COMMON_OBJS = driver/saftest_comm.o \
              driver/saftest_log.o

DRIVER_OBJS = $(COMMON_OBJS) driver/driver_utils.o

DRIVER_LIB_OBJS = $(COMMON_OBJS) driver/driver_lib_utils.o

driver/saftest_comm.o: driver/saftest_comm.c
	$(CC) $(CFLAGS) $(SHARED_INCLUDE) -c $< -o $@

driver/saftest_log.o: driver/saftest_log.c
	$(CC) $(CFLAGS) $(SHARED_INCLUDE) -c $< -o $@

driver/driver_utils.o: driver/driver_utils.c
	$(CC) $(CFLAGS) $(SHARED_INCLUDE) -c $< -o $@

driver/driver_lib_utils.o: driver/driver_lib_utils.c
	$(CC) $(CFLAGS) $(SHARED_INCLUDE) -c $< -o $@

driver/saf_driver: driver/driver_main.c $(DRIVER_OBJS)
	$(CC) $(CFLAGS) $(SHARED_INCLUDE) $(LIBS) $^ -o $@ $(LDFLAGS) 

$(CLM_DIR)/driver/clm_driver.o: $(CLM_DIR)/driver/clm_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(SHARED_INCLUDE) -c $< -o $@

$(CLM_DIR)/driver/clm_driver.so: $(CLM_DIR)/driver/clm_driver.o \
                                 $(DRIVER_LIB_OBJS)
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaClm 

$(LCK_DIR)/driver/lck_driver.o: $(LCK_DIR)/driver/lck_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(SHARED_INCLUDE) -c $< -o $@

$(LCK_DIR)/driver/lck_driver.so: $(LCK_DIR)/driver/lck_driver.o \
                                 $(DRIVER_LIB_OBJS)
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaLck

$(MSG_DIR)/driver/msg_driver.o: $(MSG_DIR)/driver/msg_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(SHARED_INCLUDE) -c $< -o $@

$(MSG_DIR)/driver/msg_driver.so: $(MSG_DIR)/driver/msg_driver.o \
                                 $(DRIVER_LIB_OBJS)
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaMsg

$(TIMEOUT_CMD):$(TIMEOUT_CMD).c
	@echo Building timeout helper files; \
	$(CC) -O2 -o $@ $< ; \

clean:
	@rm -f $(TIMEOUT_CMD)
	@rm -f driver/saf_driver
	@find . -name "*.o" -print | xargs rm -f
	@find . -name "*.so" -print | xargs rm -f
