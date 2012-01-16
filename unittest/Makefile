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
	$(DB_LIBS) \
	$(OS_LIBS) \
	$(XML_LIBS) \
	$(SQLITE_LIBS)

BSRCS = ContextUnitTest.cpp \
	ExpDecayingAvgUnitTest.cpp \
	GUIDGeneratorUnitTest.cpp \
	ProcFileSystemUnitTest.cpp \
	StateMachineUnitTest1.cpp \
	StateMachineUnitTest2.cpp \
	StateMachineUnitTest3.cpp \
	StateMachineUnitTest4.cpp \
	StateMachineUnitTest5.cpp \
	StateMachineUnitTest6.cpp \
	StateMachineUnitTest7.cpp \
	StateMachineUnitTest8.cpp

GSRCS = ActiveObjectUnitTest.cpp \
	CheckedValueStoreUnitTest.cpp \
	CheckedValueUnitTest.cpp \
	ClockUnitTest.cpp \
	ClusterLockUnitTest.cpp \
	ConditionUnitTest.cpp \
	ContextPredicateUnitTest.cpp \
	DbLiteConnectionUnitTest.cpp \
	DbConnectionPoolUnitTest.cpp \
	DbMirroredConnectionUnitTest.cpp \
	ExceptionUnitTest.cpp \
	FileSystemUnitTest.cpp \
	FileSystemUtilUnitTest.cpp \
	FStringUnitTest.cpp \
	INotifyUnitTest.cpp \
	PidFileUnitTest.cpp \
	PDUPeerUnitTest.cpp \
	ProcessManagerUnitTest.cpp \
	RunLoopUnitTest.cpp \
	SCSIUtilUnitTest.cpp \
	StateMachineDoIntervalUnitTest.cpp \
	StateMachineSetStateUnitTest.cpp \
	StateMachineTestHarnessUnitTest.cpp \

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
	@$(CCC) -o $@ $@.o \
	$(FORTE) \
	$(FORTE_MOCK) \
	$(INCLUDE) \
	$(LIBS) \
	$(BOOST_UNIT_TEST_LIB_STATIC) \


# the dependencies in this rule are wrong:
$(TARGETDIR)/ContextPredicateUnitTest: ContextPredicateUnitTest.cpp

$(TARGETDIR)/FileSystemUnitTest: FileSystemUnitTest.cpp \
	../$(TARGETDIR)/FileSystem.o 

$(TARGETDIR)/FileSystemUtilUnitTest: FileSystemUtilUnitTest.cpp \
	 ../$(TARGETDIR)/FileSystemUtil.o \
	../$(TARGETDIR)/FileSystemUtil.o \
	../$(TARGETDIR)/FileSystem.o

$(TARGETDIR)/ExceptionUnitTest: ExceptionUnitTest.cpp \
	../$(TARGETDIR)/Exception.o

$(TARGETDIR)/DbLiteConnectionUnitTest: DbLiteConnectionUnitTest.cpp $(FORTE)

$(TARGETDIR)/DbConnectionPoolUnitTest: DbConnectionPoolUnitTest.cpp $(FORTE)

$(TARGETDIR)/DbMirroredConnectionUnitTest: DbMirroredConnectionUnitTest.cpp $(FORTE)

$(TARGETDIR)/INotifyUnitTest: INotifyUnitTest.cpp \
	../$(TARGETDIR)/INotify.o

$(TARGETDIR)/FStringUnitTest: FStringUnitTest.cpp \
	../$(TARGETDIR)/FString.o 

$(TARGETDIR)/ProcessManagerUnitTest: ProcessManagerUnitTest.cpp \
	../$(TARGETDIR)/ProcessManagerImpl.o \
	../$(TARGETDIR)/ProcessFutureImpl.o \

$(TARGETDIR)/PDUPeerUnitTest: PDUPeerUnitTest.cpp $(FORTE)

$(TARGETDIR)/ClockUnitTest: ClockUnitTest.cpp $(FORTE)

$(TARGETDIR)/ClusterLockUnitTest: ClusterLockUnitTest.cpp $(FORTE)

$(TARGETDIR)/ConditionUnitTest: ConditionUnitTest.cpp $(FORTE)

$(TARGETDIR)/ActiveObjectUnitTest: ActiveObjectUnitTest.cpp $(FORTE)

$(TARGETDIR)/RunLoopUnitTest: RunLoopUnitTest.cpp $(FORTE)

$(TARGETDIR)/StateMachineSetStateUnitTest: StateMachineSetStateUnitTest.cpp \
	../$(TARGETDIR)/StateHistoryLogger.o \
	$(FORTE)

$(TARGETDIR)/StateMachineTestHarnessUnitTest: StateMachineTestHarnessUnitTest.cpp \
	$(FORTE)

$(TARGETDIR)/SCSIUtilUnitTest: SCSIUtilUnitTest.cpp $(FORTE)

$(TARGETDIR)/StateMachineDoIntervalUnitTest: StateMachineDoIntervalUnitTest.cpp $(FORTE)

$(TARGETDIR)/CheckedValueStoreUnitTest: CheckedValueStoreUnitTest.cpp ../$(TARGETDIR)/CheckedValueStore.o 

$(TARGETDIR)/CheckedValueUnitTest: CheckedValueUnitTest.cpp ../$(TARGETDIR)/CheckedValue.o 

$(TARGETDIR)/PidFileUnitTest: PidFileUnitTest.cpp ../$(TARGETDIR)/PidFile.o 

$(GPROGS): 
	@echo "Building $@ from $<..."
	$(QUIET)$(CCC) $^ -o $@ \
	$(INCLUDE) \
	$(FORTE_MOCK_LIBS) \
	$(LIBS) \
	$(FORTE_MOCK_INCLUDE) \
	$(GTEST_LIB)


# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
