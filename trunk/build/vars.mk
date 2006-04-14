LIBS = $(IMPLEMENTATION_LIBS)

# Define our directories
# SAFTEST is the root of the saftest source
# OBJDIR is a temporary location for intermediate object files
# FINAL_OBJDIR is the location of finalized programs, libraries, etc

SAFTEST:=$(shell pwd)
SAFTEST_VERSION=1.00
SAFTEST_RELEASE=0
OBJDIR=objs
FINAL_OBJDIR=$(OBJDIR)/final

standard_include = -Iinclude/standard

shared_include = -Iinclude -Iinclude/$(PLATFORM) $(PLATFORM_INCLUDES) $(standard_include)
