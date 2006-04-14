$(OBJDIR)/clm_driver.o: $(CLM_DIR)/driver/clm_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/clm_driver.so: $(OBJDIR)/clm_driver.o $(driver_lib_objs)
	$(LD) $(LIBS) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS) -lSaClm

test_libs += $(FINAL_OBJDIR)/clm_driver.so
