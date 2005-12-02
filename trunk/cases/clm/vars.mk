CLM_DIR=$(SAFTEST)/cases/clm

$(OBJDIR)/clm_driver.o: $(CLM_DIR)/driver/clm_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/clm_driver.so: $(OBJDIR)/clm_driver.o \
                                 $(driver_lib_objs)
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaClm

test_libs += $(FINAL_OBJDIR)/clm_driver.so
