MSG_DIR=$(SAFTEST)/cases/msg

$(OBJDIR)/msg_driver.o: $(MSG_DIR)/driver/msg_driver.c
	$(CC) $(CFLAGS) $(LIBS) $(shared_include) -c $< -o $@

$(FINAL_OBJDIR)/msg_driver.so: $(OBJDIR)/msg_driver.o \
                                 $(driver_lib_objs)
	$(LD) $(LIBS) -shared -o $@ $^ $(LDFLAGS) -lSaMsg

test_libs += $(FINAL_OBJDIR)/msg_driver.so
