# Copyright (C) 2010 Scale Computing, Inc.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall -ggdb

INCLUDE = \
	-I. \
	-I.. \
	-I../gen-cpp \
	$(GTEST_INCLUDE) \
	$(FORTE_INCLUDE) \
	$(LIBSCALE_INCLUDE) \
	$(DB_INCLUDE) \
	$(SOAP_INCLUDE) \
	$(BOOST_INCLUDE) \

LIBS = \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(LIBSCALE_LIBS) \
	$(LIBSQLITE) \
	$(BOOST_LIBS) \
	$(BOOST_FS_LIB) \
	-lpthread -lrt

PROGS=$(TARGETDIR)/MockFileSystemUnitTest $(TARGETDIR)/MockProcessManagerUnitTest\

#	$(TARGETDIR)/TestRunInstanceUnitTest \


PROG_DEPS_OBJS_MockFileSystemUnitTest = \
	../$(TARGETDIR)/MockFileSystem.o \

PROG_DEPS_OBJS_MockProcessManagerUnitTest = \
	../$(TARGETDIR)/MockProcessManager.o \

###  RULES  ###

all: $(FORTE_MOCK) $(THRIFT_LIB) $(GMOCK) $(GTEST) $(PROGS)

$(foreach p,$(subst $(TARGETDIR)/,,$(PROGS)),$(eval $(call GENERATE_LINK_TEST_RULE,$(p))))

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all mockobjects
