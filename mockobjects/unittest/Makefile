# Copyright (C) 2010 Scale Computing, Inc.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall -ggdb

COMMON_INCLUDE = \
	-I. \
	-I.. \
	-I../gen-cpp \
	$(GTEST_INCLUDE) \
	$(FORTE_INCLUDE) \
	$(LIBSCALE_INCLUDE) \
	$(DB_INCLUDE) \
	$(SOAP_INCLUDE) \
	$(BOOST_INCLUDE) \

COMMON_LIBS = \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(LIBSCALE_LIBS) \
	$(LIBSQLITE) \
	$(BOOST_LIBS) \
	$(BOOST_FS_LIB) \
	-lpthread -lrt

PROGS=$(TARGETDIR)/MockFileSystemUnitTest $(TARGETDIR)/MockProcessManagerUnitTest\

#	$(TARGETDIR)/TestRunInstanceUnitTest \

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###

all: parent $(FORTE_MOCK) $(THRIFT_LIB) $(GMOCK) $(GTEST) $(PROGS) 

parent: mockobjects FORCE
	$(MAKE) -C ../


$(TARGETDIR)/MockFileSystemUnitTest: ../$(TARGETDIR)/MockFileSystem.o MockFileSystemUnitTest.cpp
	echo "Building $@..."
	$(CCC) MockFileSystemUnitTest.cpp \
	-o $(TARGETDIR)/MockFileSystemUnitTest \
	../$(TARGETDIR)/MockFileSystem.o \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS) \
	$(FORTE_MOCK_INCLUDE) \
	$(FORTE_MOCK_LIBS) \

$(TARGETDIR)/MockProcessManagerUnitTest: ../$(TARGETDIR)/MockProcessManager.o MockProcessManagerUnitTest.cpp
	echo "Building $@..."
	$(CCC) MockProcessManagerUnitTest.cpp \
	-o $(TARGETDIR)/MockProcessManagerUnitTest \
	../$(TARGETDIR)/MockProcessManager.o \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS) \
	$(FORTE_MOCK_INCLUDE) \
	$(FORTE_MOCK_LIBS) \

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all mockobjects
