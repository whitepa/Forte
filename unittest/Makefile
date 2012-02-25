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

DEFS += -DFORTE_WITH_SQLITE

INCLUDE = \
	-I. \
	-I../mockobjects \
	$(FORTE_INCLUDE) \
	$(GTEST_INCLUDE) \
	$(DB_INCLUDE)

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_MOCK_LIBS) \
	$(FORTE_DB_SQLITE) \
	$(FORTE_LIBS) \
	$(FORTE_DB_SQLITE) \
	$(BOOST_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_REGEX_LIB) \
	$(CURL_LIBS) \
	$(OS_LIBS) \
	$(XML_LIBS) \
	$(SQLITE_LIBS) \
	$(NULL)

BSRCS = ContextUnitTest.cpp \
	ExpDecayingAvgUnitTest.cpp \
	GUIDGeneratorUnitTest.cpp \
	StateMachineUnitTest1.cpp \
	StateMachineUnitTest2.cpp \
	StateMachineUnitTest3.cpp \
	StateMachineUnitTest4.cpp \
	StateMachineUnitTest5.cpp \
	StateMachineUnitTest6.cpp \
	StateMachineUnitTest7.cpp \
	StateMachineUnitTest8.cpp

GSRCS = CheckedValueStoreUnitTest.cpp \
	CheckedValueUnitTest.cpp \
	ClockUnitTest.cpp \
	ClusterLockUnitTest.cpp \
	ConditionUnitTest.cpp \
	ConditionFileLoggerUnitTest.cpp \
	ConditionAsyncLoggerUnitTest.cpp \
	ConditionDeltaLoggerUnitTest.cpp \
	ContextPredicateUnitTest.cpp \
	DbLiteConnectionUnitTest.cpp \
	DbConnectionPoolUnitTest.cpp \
	DbMirroredConnectionUnitTest.cpp \
	ExceptionUnitTest.cpp \
	FileSystemImplUnitTest.cpp \
	FileSystemUtilUnitTest.cpp \
	FStringUnitTest.cpp \
	INotifyUnitTest.cpp \
	PidFileUnitTest.cpp \
	PDUPeerUnitTest.cpp \
	ProcessManagerUnitTest.cpp \
	ProcFileSystemUnitTest.cpp \
	SCSIUtilUnitTest.cpp \
	StateMachineDoIntervalUnitTest.cpp \
	StateMachineSetStateUnitTest.cpp \
	StateMachineTestHarnessUnitTest.cpp \

GPROGS = $(GSRCS:%.cpp=%)
BPROGS = $(BSRCS:%.cpp=%)
PROGS=$(addprefix $(TARGETDIR)/,$(GPROGS) $(BPROGS))

PROG_DEPS = $(FORTE) $(FORTE_MOCK)

PROG_DEPS_OBJS_ContextPredicateUnitTest =

PROG_DEPS_OBJS_FileSystemImplUnitTest =  \
	../$(TARGETDIR)/FileSystemImpl.o

PROG_DEPS_OBJS_FileSystemUtilUnitTest =  \
	../$(TARGETDIR)/FileSystemUtil.o \
	../$(TARGETDIR)/FileSystemImpl.o

PROG_DEPS_OBJS_ExceptionUnitTest =  \
	../$(TARGETDIR)/Exception.o

PROG_DEPS_OBJS_DbLiteConnectionUnitTest =

PROG_DEPS_OBJS_DbConnectionPoolUnitTest =

PROG_DEPS_OBJS_DbMirroredConnectionUnitTest =

PROG_DEPS_OBJS_INotifyUnitTest =  \
	../$(TARGETDIR)/INotify.o

PROG_DEPS_OBJS_FStringUnitTest =  \
	../$(TARGETDIR)/FString.o

PROG_DEPS_OBJS_ProcessManagerUnitTest =  \
	../$(TARGETDIR)/ProcessManagerImpl.o \
	../$(TARGETDIR)/ProcessFutureImpl.o \

PROG_DEPS_OBJS_ProcFileSystemUnitTest =  \
	../$(TARGETDIR)/ProcFileSystem.o \

PROG_DEPS_OBJS_PDUPeerUnitTest =

PROG_DEPS_OBJS_ClockUnitTest =

PROG_DEPS_OBJS_ClusterLockUnitTest =

PROG_DEPS_OBJS_ConditionUnitTest =

PROG_DEPS_OBJS_ConditionFileLoggerUnitTest =  \
	../$(TARGETDIR)/ConditionFileLogger.o \
	../$(TARGETDIR)/ConditionLogEntry.o \

PROG_DEPS_OBJS_ConditionAsyncLoggerUnitTest =  \
	../$(TARGETDIR)/ConditionAsyncLogger.o \
	../$(TARGETDIR)/ConditionLogEntry.o \

PROG_DEPS_OBJS_ConditionDeltaLoggerUnitTest =  \
	../$(TARGETDIR)/ConditionDeltaLogger.o \
	../$(TARGETDIR)/ConditionLogEntry.o \

PROG_DEPS_OBJS_ActiveObjectUnitTest =

PROG_DEPS_OBJS_StateMachineSetStateUnitTest =  \
	../$(TARGETDIR)/StateHistoryLogger.o \

PROG_DEPS_OBJS_StateMachineTestHarnessUnitTest =

PROG_DEPS_OBJS_SCSIUtilUnitTest =

PROG_DEPS_OBJS_StateMachineDoIntervalUnitTest =

PROG_DEPS_OBJS_CheckedValueStoreUnitTest =  ../$(TARGETDIR)/CheckedValueStore.o

PROG_DEPS_OBJS_CheckedValueUnitTest =  ../$(TARGETDIR)/CheckedValue.o

PROG_DEPS_OBJS_PidFileUnitTest =  ../$(TARGETDIR)/PidFile.o

###  RULES  ###

all: $(PROGS)

# progs
$(foreach p,$(BPROGS),$(eval $(call GENERATE_LINK_TEST_RULE,$(p),$(BOOST_UNIT_TEST_LIB_STATIC))))

$(foreach p,$(GPROGS),$(eval $(call GENERATE_LINK_TEST_RULE,$(p),$(GMOCK_LIBS) $(GTEST_LIB))))

# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
