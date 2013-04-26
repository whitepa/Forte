# Makefile for unit tests
# Copyright (C) 2008 Alchemy Technology Partners, LLC
# Copyright (C) 2010 Scale Computing, Inc.

# To add a new test, create a new .cpp file in this directory.  It
# will be compiled as a standalone executable. If the the test has
# dependencies outside of what is in LIBS, add them individually via
# the PROG_DEPS_OBJS_<test> variable below.

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
	$(AIO_LIBS) \
	$(NULL)

BSRCS = ContextUnitTest.cpp \
	ExpDecayingAvgUnitTest.cpp \
	StateMachineUnitTest1.cpp \
	StateMachineUnitTest2.cpp \
	StateMachineUnitTest4.cpp \
	StateMachineUnitTest5.cpp \
	StateMachineUnitTest6.cpp \
	StateMachineUnitTest7.cpp \
	StateMachineUnitTest8.cpp

# onbox now due to time dependency
# StateMachineUnitTest3.cpp \

GSRCS = AutoMutexUnitTest.cpp \
	CheckedValueStoreUnitTest.cpp \
	CheckedValueUnitTest.cpp \
	ClockUnitTest.cpp \
	CollectorUnitTest.cpp \
	ConditionUnitTest.cpp \
	ConditionFileLoggerUnitTest.cpp \
	ConditionAsyncLoggerUnitTest.cpp \
	ConditionDeltaLoggerUnitTest.cpp \
	ContextPredicateUnitTest.cpp \
	DbLiteConnectionUnitTest.cpp \
	DbConnectionPoolUnitTest.cpp \
	ExceptionUnitTest.cpp \
	FileSystemUtilUnitTest.cpp \
	FStringUnitTest.cpp \
	FTraceUnitTest.cpp \
	GUIDGeneratorUnitTest.cpp \
	INotifyUnitTest.cpp \
	IOManagerUnitTest.cpp \
	LoggingUnitTest.cpp \
	LogManagerUnitTest.cpp \
	OnDemandDispatcherUnitTest.cpp \
	PDUPeerFileDescriptorEndpointUnitTest.cpp \
	PDUPeerImplUnitTest.cpp \
	PDUPeerInProcessEndpointUnitTest.cpp \
	PDUPeerNetworkAcceptorEndpointUnitTest.cpp \
	PDUPeerNetworkConnectorEndpointUnitTest.cpp \
	PDUUnitTest.cpp \
	PidFileUnitTest.cpp \
	ProcessManagerUnitTest.cpp \
	ProcessCommandUnitTest.cpp \
	ProcessorInformationUnitTest.cpp \
	ProcFileSystemUnitTest.cpp \
	RWLockUnitTest.cpp \
	SCSIUtilUnitTest.cpp \
	StateMachineDoIntervalUnitTest.cpp \
	StateMachineDoMaximumIterationsUnitTest.cpp \
	StateMachineEventDeliveryUnitTest.cpp \
	StateMachineEventDeliveryUnitTest2.cpp \
	StateMachineSetStateUnitTest.cpp \
	StateMachineTestHarnessUnitTest.cpp \
	ThreadPoolDispatcherUnitTest.cpp \
	XMLUnitTest.cpp \
	EnableStatsUnitTest.cpp \

# flaky
#	DbMirroredConnectionUnitTest.cpp \

GPROGS = $(GSRCS:%.cpp=%)
BPROGS = $(BSRCS:%.cpp=%)
PROGS=$(addprefix $(TARGETDIR)/,$(GPROGS) $(BPROGS))

PROG_DEPS = $(FORTE_MOCK) $(FORTE)

PROG_DEPS_OBJS_AutoMutexUnitTest =

PROG_DEPS_OBJS_ContextPredicateUnitTest =

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
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerSetImpl.o \
	../$(TARGETDIR)/PDUPeerFileDescriptorEndpoint.o \

PROG_DEPS_OBJS_ProcessCommandUnitTest =

PROG_DEPS_OBJS_ProcFileSystemUnitTest =  \
	../$(TARGETDIR)/ProcFileSystem.o \

PROG_DEPS_OBJS_ProcessorInformationUnitTest =  \
	../$(TARGETDIR)/ProcFileSystem.o \
	../$(TARGETDIR)/ProcessorInformation.o \

PROG_DEPS_OBJS_PDUPeerInProcessEndpointUnitTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerInProcessEndpoint.o \

PROG_DEPS_OBJS_PDUPeerFileDescriptorEndpointUnitTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerFileDescriptorEndpoint.o \

PROG_DEPS_OBJS_PDUPeerNetworkAcceptorEndpointUnitTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerNetworkAcceptorEndpoint.o \

PROG_DEPS_OBJS_PDUPeerNetworkConnectorEndpointUnitTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerNetworkConnectorEndpoint.o \
	../$(TARGETDIR)/Thread.o \

PROG_DEPS_OBJS_PDUPeerImplUnitTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerImpl.o \

PROG_DEPS_OBJS_PDUUnitTest = \
	../$(TARGETDIR)/PDU.o \

PROG_DEPS_OBJS_ClockUnitTest =

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

PROG_DEPS_OBJS_ThreadPoolDispatcherUnitTest = \
	../$(TARGETDIR)/ThreadPoolDispatcher.o \

PROG_DEPS_OBJS_RWLockUnitTest = \
	../$(TARGETDIR)/RWLock.o \

PROG_DEPS_OBJS_SCSIUtilUnitTest =

PROG_DEPS_OBJS_StateMachineDoIntervalUnitTest =

PROG_DEPS_OBJS_CheckedValueStoreUnitTest =  ../$(TARGETDIR)/CheckedValueStore.o

PROG_DEPS_OBJS_CheckedValueUnitTest =  ../$(TARGETDIR)/CheckedValue.o

PROG_DEPS_OBJS_CollectorUnitTest = ../$(TARGETDIR)/CheckedValueStore.o \
				   ../$(TARGETDIR)/Collector.o

PROG_DEPS_OBJS_PidFileUnitTest =  ../$(TARGETDIR)/PidFile.o

PROG_DEPS_OBJS_XMLUnitTest =

PROG_DEPS_OBJS_EnableStatsUnitTest =  

###  RULES  ###

all: $(PROGS)

# progs
$(foreach p,$(BPROGS),$(eval $(call GENERATE_LINK_TEST_RULE,$(p),$(BOOST_UNIT_TEST_LIB_STATIC))))

$(foreach p,$(GPROGS),$(eval $(call GENERATE_LINK_TEST_RULE,$(p),$(GMOCK_LIBS) $(GTEST_LIB))))

# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
