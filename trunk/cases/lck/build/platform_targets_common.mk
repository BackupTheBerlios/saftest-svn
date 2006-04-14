$(OBJDIR)/lck_driver.o: $(LCK_DIR)/driver/lck_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/lck_driver.so: $(OBJDIR)/lck_driver.o $(driver_lib_objs)
	$(LD) $(LIBS) $(SHLIB_LDFLAGS) $@ $^ $(LDFLAGS) -lSaLck

test_libs += $(FINAL_OBJDIR)/lck_driver.so
