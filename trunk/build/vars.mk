LIBS = $(IMPLEMENTATION_LIBS)

# Define our directories
# SAFTEST is the root of the saftest source
# SAF_OBJDIR is a temporary location for intermediate object files
# FINAL_SAF_OBJDIR is the location of finalized programs, libraries, etc

SAFTEST:=$(shell pwd)
SAFTEST_VERSION=1.00
SAFTEST_RELEASE=0
SAF_OBJDIR=objs
FINAL_SAF_OBJDIR=$(SAF_OBJDIR)/final

saftest_release := $(SAFTEST_RELEASE)$(rpm_style)
ifeq ($(DISTRO), redhat)
    saftest_release := $(saftest_release).rhel$(rh_ver)$(rpm_style)
endif
ifeq ($(DISTRO), suse)
    saftest_release := $(saftest_release).sles$(sles_ver)$(rpm_style)
endif

standard_include = -Iinclude/standard

shared_include = -Iinclude -Iinclude/$(PLATFORM) $(PLATFORM_INCLUDES) $(standard_include)
