.PHONY: saftestrpm

RPMCMD=/usr/bin/rpmbuild
RPM_ARCH=i386

# IA64 rpm does not accept --target option
ifeq ($(PKG_ARCH), i386)
    rpm_target := --target=$(PKG_ARCH)
endif

RPM_DEFINES=--define 'saftest_version $(SAFTEST_VERSION)' \
            --define 'saftest_release $(SAFTEST_RELEASE)' \
            --define 'rpm_arch $(PKG_ARCH)' \
            --define 'distro $(DISTRO)' \
            --define '_topdir $(SAFTEST)' \
            --define 'buildobjdir $(SAF_OBJDIR)'

saftestrpm: $(FINAL SAF_OBJDIR)/saftest-$(SAFTEST_VERSION)-$(SAFTEST_RELEASE).$(RPM_ARCH).rpm

$(FINAL SAF_OBJDIR)/saftest-$(SAFTEST_VERSION)-$(SAFTEST_RELEASE).$(RPM_ARCH).rpm: build/linux/rpm/saftest.spec saftest
	$(RPMCMD) -bb --quiet $(rpm_target) $(RPM_DEFINES) $<
