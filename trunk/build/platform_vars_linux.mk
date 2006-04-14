CFLAGS = -g -fPIC -Wall -Werror -D__AIS_B_TEST__
LDFLAGS := -ldl -lpthread
SHLIB_LDFLAGS += -shared -o

DISTRO= $(shell if [ -f /etc/SuSE-release ]; \
                then echo suse; \
                elif [ -f /etc/debian_version ]; \
                then echo debian;\
                else \
                echo redhat; fi)

PKG_ARCH=$(shell uname -m)
ifeq ($(PKG_ARCH), i686)
    PKG_ARCH=i386
endif
