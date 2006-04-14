%/dir_exists:
	mkdir -p $(@D)
	@echo "This file created for make to use." > $@
	@echo "It proves directory existance." >> $@
	@echo "Its timestamp is more stable." >> $@

common_objs = $(OBJDIR)/saftest_comm.o \
              $(OBJDIR)/saftest_log.o \
              $(OBJDIR)/saftest_list.o \
              $(OBJDIR)/saftest_getopt.o \
              $(OBJDIR)/saftest_getoptl.o

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

$(OBJDIR)/saftest_list.o: driver/saftest_list.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(OBJDIR)/saftest_getopt.o: driver/saftest_getopt.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(OBJDIR)/saftest_getoptl.o: driver/saftest_getoptl.c
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/saf_driver: driver/driver_main.c $(driver_objs)
	$(CC) $(CFLAGS) $(shared_include) $(LIBS) $^ -o $@ $(LDFLAGS)
