driver_lib_objs64 = $(addprefix $(SAF_OBJDIR64)/,$(notdir $(common_srcs))) $(SAF_OBJDIR64)/driver_lib_utils.o

$(SAF_OBJDIR64)/clm_driver.o: $(CLM_DIR)/driver/clm_driver.c
	$(CC) $(CFLAGS64) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR64)/clm_driver.so: $(SAF_OBJDIR64)/clm_driver.o $(driver_lib_objs64)
	$(LD) $(LIBS64) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS) -lSaClm

test_libs64 += $(FINAL_SAF_OBJDIR64)/clm_driver.so
