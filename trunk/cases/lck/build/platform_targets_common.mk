$(SAF_OBJDIR)/lck_driver.o: $(LCK_DIR)/driver/lck_driver.c
#$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@
	$(CC) $(CFLAGS) $(shared_include) -c $< -o $@

$(FINAL_SAF_OBJDIR)/lck_driver.so: $(SAF_OBJDIR)/lck_driver.o $(driver_lib_objs)
	$(LD) $(LIBS) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS) -lSaLck

test_libs += $(FINAL_SAF_OBJDIR)/lck_driver.so
