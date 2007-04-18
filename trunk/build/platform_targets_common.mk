%/dir_exists:
	mkdir -p $(@D)
	@echo "This file created for make to use." > $@
	@echo "It proves directory existance." >> $@
	@echo "Its timestamp is more stable." >> $@

common_srcs = driver/saftest_comm.o \
              driver/saftest_log.o \
              driver/saftest_list.o \
              driver/saftest_getopt.o \
              driver/saftest_getoptl.o \
              driver/driver_utils.o \
              driver/driver_lib_utils.o

driver_lib_objs = $(addprefix $(SAF_OBJDIR)/,$(notdir $(common_srcs))) $(SAF_OBJDIR)/driver_lib_utils.o

$(SAF_OBJDIR)/%.o: driver/%.c $(SAF_OBJDIR)/dir_exists
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR)/saf_driver: $(addprefix $(SAF_OBJDIR)/,$(patsubst %.c,%.o,$(notdir driver/driver_main.c))) \
	$(addprefix $(SAF_OBJDIR)/,$(notdir $(common_srcs)))
	$(CC) $(CFLAGS) $(shared_include) $(LIBS) $^ -o $@ $(LDFLAGS)

$(SAF_OBJDIR)/saftest_main_lib.o: driver/saftest_main_lib.c
	$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR)/saftest_main_lib.so: $(SAF_OBJDIR)/saftest_main_lib.o $(driver_lib_objs)
	$(LD) $(LIBS) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS)

