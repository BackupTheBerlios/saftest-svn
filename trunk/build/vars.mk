CFLAGS = -g -Wall -Werror -D__AIS_B_TEST__

LIBS = -L/usr/local/glib-2.6.3/lib $(IMPLEMENTATION_LIBS)

# Define our directories
# SAFTEST is the root of the saftest source
# OBJDIR is a temporary location for intermediate object files
# FINAL_OBJDIR is the location of finalized programs, libraries, etc

SAFTEST=$(shell pwd)
OBJDIR=$(SAFTEST)/objs
FINAL_OBJDIR=$(OBJDIR)/final

%/dir_exists:
	mkdir -p $(@D)
	@echo "This file created for make to use." > $@
	@echo "It proves directory existance." >> $@
	@echo "Its timestamp is more stable." >> $@

standard_include = -Iinclude/standard

shared_include = -I/usr/local/glib-2.6.3/include/glib-2.0 \
                 -I/usr/local/glib-2.6.3/lib/glib-2.0/include \
                 -Iinclude $(standard_include)

common_objs = $(OBJDIR)/saftest_comm.o \
              $(OBJDIR)/saftest_log.o

driver_objs = $(common_objs) $(OBJDIR)/driver_utils.o

driver_lib_objs = $(common_objs) $(OBJDIR)/driver_lib_utils.o

$(OBJDIR)/saftest_comm.o: driver/saftest_comm.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(OBJDIR)/saftest_log.o: driver/saftest_log.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(OBJDIR)/driver_utils.o: driver/driver_utils.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(OBJDIR)/driver_lib_utils.o: driver/driver_lib_utils.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/saf_driver: driver/driver_main.c $(driver_objs)
	$(CC) $(CFLAGS) $(shared_include) $(LIBS) $^ -o $@ $(LDFLAGS)

