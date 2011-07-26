# Makefile for unit tests
# Copyright (C) 2008 Alchemy Technology Partners, LLC
# Copyright (C) 2010 Scale Computing, Inc.

# To add a new test, create a new .cpp file in this directory.  It
# will be compiled as a standalone executable.

# TODO: This makefile does not appear to handle dependencies
# correctly.  If library headers change, it does not recompile the
# individual tests.

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
	$(GTEST_INCLUDE) \
	$(DB_INCLUDE)

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
	GUIDGeneratorUnitTest.cpp \
	ProcFileSystemUnitTest.cpp \
	StateMachineUnitTest1.cpp \
	StateMachineUnitTest2.cpp \
	StateMachineUnitTest3.cpp \
	StateMachineUnitTest4.cpp \
	StateMachineUnitTest5.cpp \
	StateMachineUnitTest6.cpp \
	StateMachineUnitTest7.cpp

GSRCS = ActiveObjectUnitTest.cpp \
	CheckedValueStoreUnitTest.cpp \
	ClusterLockUnitTest.cpp \
	DbLiteConnectionUnitTest.cpp \
	ExceptionUnitTest.cpp \
	FileSystemUnitTest.cpp \
	FileSystemUtilUnitTest.cpp \
	FStringUnitTest.cpp \
	PDUPeerUnitTest.cpp \
	ProcessManagerUnitTest.cpp \
	RunLoopUnitTest.cpp \
	SCSIUtilUnitTest.cpp

GPROGS = $(GSRCS:%.cpp=$(TARGETDIR)/%)
BPROGS = $(BSRCS:%.cpp=$(TARGETDIR)/%)
PROGS=$(GPROGS) $(BPROGS)

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###

all: $(PROGS) 

# progs
$(BPROGS): $(TARGETDIR)/%: $(FORTE) $(FORTE_MOCK) %.cpp $(TARGETDIR)/%.o
	@echo "Linking $@ ..."
	@$(CCC) -o $@ $@.o $(LIBS)

# the dependencies in this rule are wrong:
$(TARGETDIR)/FileSystemUnitTest: ../$(TARGETDIR)/FileSystem.o FileSystemUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) FileSystemUnitTest.cpp \
	-o $@ \
	../$(TARGETDIR)/FileSystem.o \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/FileSystemUtilUnitTest: ../$(TARGETDIR)/FileSystemUtil.o FileSystemUtilUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) FileSystemUtilUnitTest.cpp \
	-o $@ \
	../$(TARGETDIR)/FileSystemUtil.o \
	../$(TARGETDIR)/FileSystem.o \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/ExceptionUnitTest: ../$(TARGETDIR)/Exception.o ExceptionUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) ExceptionUnitTest.cpp \
	-o $@ \
	$(FORTE) \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)


$(TARGETDIR)/DbLiteConnectionUnitTest: DbLiteConnectionUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) DbLiteConnectionUnitTest.cpp \
	-o $@ \
	$(FORTE) \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS) \
	-lsqlite3

$(TARGETDIR)/FStringUnitTest: ../$(TARGETDIR)/FString.o FStringUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) FStringUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/ProcessManagerUnitTest: ../$(TARGETDIR)/ProcessManager.o ProcessManagerUnitTest.cpp $(FORTE)
	@echo "Compiling $@..."
	$(QUIET)$(CCC) ProcessManagerUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/PDUPeerUnitTest: PDUPeerUnitTest.cpp $(FORTE)
	@echo "Compiling $@..."
	$(QUIET)$(CCC) PDUPeerUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(OS_LIBS) \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(FORTE_MOCK_LIBS) \

$(TARGETDIR)/ClusterLockUnitTest: ClusterLockUnitTest.cpp $(FORTE)
	@echo "Compiling $@..."
	$(QUIET)$(CCC) ClusterLockUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(OS_LIBS) \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(FORTE_MOCK_LIBS) \
	$(BOOST_FS_LIB) \

$(TARGETDIR)/ActiveObjectUnitTest: ActiveObjectUnitTest.cpp $(FORTE)
	@echo "Compiling $@..."
	$(QUIET)$(CCC) ActiveObjectUnitTest.cpp \
	-o $@ \
	$(INCLUDE) \
	$(OS_LIBS) \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(FORTE_MOCK_LIBS) \
	$(BOOST_FS_LIB) \

$(TARGETDIR)/RunLoopUnitTest: RunLoopUnitTest.cpp $(FORTE)
	echo "Building $@..."
	$(QUIET)$(CCC) $< \
	-o $@ \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/SCSIUtilUnitTest: SCSIUtilUnitTest.cpp
	@echo "Compiling $@..."
	$(QUIET)$(CCC) SCSIUtilUnitTest.cpp \
	-o $(TARGETDIR)/SCSIUtilUnitTest \
	$(SCALE_SERVICE_CLIENT_LIBS)  \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

$(TARGETDIR)/CheckedValueStoreUnitTest: ../$(TARGETDIR)/CheckedValueStore.o CheckedValueStoreUnitTest.cpp
	echo "Building $@..."
	$(QUIET)$(CCC) CheckedValueStoreUnitTest.cpp \
	-o $(TARGETDIR)/CheckedValueStoreUnitTest \
	$(INCLUDE) \
	$(GTEST_LIB) \
	$(LIBS)

# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
