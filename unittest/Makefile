# Makefile for unit tests
# Copyright (C) 2008 Alchemy Technology Partners, LLC
# Copyright (C) 2010 Scale Computing, Inc.

# To add a new test, create a new .cpp file in this directory.  It
# will be compiled as a standalone executable.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall -ggdb

INCLUDE = \
	-I. \
	-I.. \
	-I../mockobjects \
	$(XML_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(GTEST_INCLUDE)

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \
	$(FORTE_MOCK_LIBS) \
	$(BOOST_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_REGEX_LIB) \
	$(CURL_LIBS) \
	$(OS_LIBS) \
	$(XML_LIBS) \
	$(BOOST_UNIT_TEST_LIB_STATIC) \


BSRCS = ContextPredicateUnitTest.cpp \
	ContextUnitTest.cpp \
	ExpDecayingAvgUnitTest.cpp \
	FileSystemUnitTest.cpp \
	ProcFileSystemUnitTest.cpp \
	StateMachineUnitTest.cpp \
	StateMachineUnitTest1.cpp
GSRCS = FileSystemUtilUnitTest.cpp \
	ProcessManagerUnitTest.cpp

GPROGS = $(GSRCS:%.cpp=$(TARGETDIR)/%)
BPROGS = $(BSRCS:%.cpp=$(TARGETDIR)/%)
PROGS=$(GPROGS) $(BPROGS)

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###

all: parent $(PROGS) 

parent: FORCE
	$(MAKE) -C ../

# progs
$(BPROGS): $(TARGETDIR)/%: $(FORTE) $(FORTE_MOCK) %.cpp $(TARGETDIR)/%.o
	@echo "Building $@ ..."
	@$(CCC) -o $@ $@.o $(LIBS)

$(TARGETDIR)/FileSystemUtilUnitTest: ../$(TARGETDIR)/FileSystemUtil.o FileSystemUtilUnitTest.cpp
	echo "Building $@..."
	$(CCC) FileSystemUtilUnitTest.cpp \
	-o $@ \
	../$(TARGETDIR)/FileSystemUtil.o \
	../$(TARGETDIR)/FileSystem.o \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/ProcessManagerUnitTest: ProcessManagerUnitTest.cpp
	echo "Building $@..."
	$(CCC) ProcessManagerUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
