.PHONY: depot saftestdepot
SAFTESTDEPOT=saftest.depot
saftestdepot:   $(SAF_OBJDIR)/$(SAFTESTDEPOT)
SWPACKAGE = swpackage
$(SAF_OBJDIR64)/%.o: driver/%.c $(SAF_OBJDIR64)/dir_exists
	$(CC) $(CFLAGS64) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR64)/saf_driver: $(addprefix $(SAF_OBJDIR64)/,$(patsubst %.c,%.o,$(notdir driver/driver_main.c))) \
	$(addprefix $(SAF_OBJDIR64)/,$(notdir $(common_srcs)))
	$(CC) $(CFLAGS64) $(shared_include) $(LIBS64) $^ -o $@ $(LDFLAGS)


driver_lib_objs64 = $(addprefix $(SAF_OBJDIR64)/,$(notdir $(common_srcs))) $(SAF_OBJDIR64)/driver_lib_utils.o

$(SAF_OBJDIR64)/saftest_main_lib.o: driver/saftest_main_lib.c
	$(CC) $(CFLAGS64) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR64)/saftest_main_lib.so: $(SAF_OBJDIR64)/saftest_main_lib.o $(driver_lib_objs64)
	$(LD) $(LIBS64) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS)

test_libs64 +=$(FINAL_SAF_OBJDIR64)/saftest_main_lib.so
saftest: $(FINAL_SAF_OBJDIR64)/dir_exists $(FINAL_SAF_OBJDIR64)/saf_driver $(test_libs64)

$(SAF_OBJDIR)/saftest.psf: /ha/sg/tests/saftest_ng/build/hpux/depot/saftest.psf $(SAF_OBJDIR)/dir_exists
	$<> $@
#$(SAF_OBJDIR)/$(SAFTESTDEPOT): /ha/sg/tests/saftest_ng/build/hpux/depot/saftest.psf
$(SAF_OBJDIR)/$(SAFTESTDEPOT): $(SAF_OBJDIR)/saftest.psf
	cd $(SAF_OBJDIR); \
	$(SWPACKAGE) $(SWPKG_OPTIONS) -d `pwd`/$(@F) -x target_type=tape \
		-s $(<F)
