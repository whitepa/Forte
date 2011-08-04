# Makefile
#
# Copyright 2008 Alchemy Technology Partners, LLC
# Proprietary and Confidential
#

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)
include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

INSTALL_ROOT ?=
PREFIX ?= $(INSTALL_ROOT)/usr/local
HEADER_INSTALL_PATH = $(PREFIX)/include
LIB_INSTALL_PATH = $(PREFIX)/lib

SUBDIRS = dbc mockobjects unittest


INCLUDE = $(DB_INCLUDE) $(XML_INCLUDE) $(BOOST_INCLUDE) $(SSH2_INCLUDE) -I. $(MYSQL_INCLUDE)

CCARGS += -Wall -DFORTE_FUNCTION_TRACING
SRCS =	\
	ActiveObjectThread.cpp \
	Base64.cpp \
	AdvisoryLock.cpp \
	ClusterLock.cpp \
	CheckedValue.cpp \
	CheckedValueStore.cpp \
	Clock.cpp \
	Collector.cpp \
	CollectorPollThread.cpp \
	Condition.cpp \
	ConditionPollThread.cpp \
	Context.cpp \
	ContextPredicate.cpp \
	Curl.cpp \
	DaemonUtil.cpp \
	DbConnection.cpp \
	DbMyConnection.cpp \
	DbPgConnection.cpp \
	DbLiteConnection.cpp \
	DbConnectionPool.cpp \
	DbException.cpp \
	DbRow.cpp \
	DbResult.cpp \
	DbMyResult.cpp \
	DbPgResult.cpp \
	DbLiteResult.cpp \
	Dispatcher.cpp \
	EventQueue.cpp \
	EventPredicate.cpp \
	Exception.cpp \
	ExpDecayingAvg.cpp \
	FileSystem.cpp \
	FileSystemUtil.cpp \
	FString.cpp \
	FTime.cpp \
	FTrace.cpp \
	GUIDGenerator.cpp \
	LogManager.cpp \
	LogTimer.cpp \
	MD5.cpp \
	Murmur.cpp \
	OnDemandDispatcher.cpp \
	OpenSSLInitializer.cpp \
	PDUPeer.cpp \
	PDUPeerSet.cpp \
	PidFile.cpp \
	PDUPeer.cpp \
	PDUPeerSet.cpp \
	PosixTimer.cpp \
	ProcFileSystem.cpp \
	ProcRunner.cpp \
	ProcessFutureImpl.cpp \
	ProcessManagerImpl.cpp \
	Random.cpp \
	RandomGenerator.cpp \
	ReceiverThread.cpp \
	RunLoop.cpp \
	RWLock.cpp \
	SCSIUtil.cpp \
	SecureEnvelope.cpp \
	SecureString.cpp \
	ServerMain.cpp \
	ServiceConfig.cpp \
	SSHRunner.cpp \
	State.cpp \
	StateMachine.cpp \
	StateRegion.cpp \
	Thread.cpp \
	ThreadKey.cpp \
	ThreadPoolDispatcher.cpp \
	Timer.cpp \
	UrlString.cpp \
	Util.cpp \
	VersionManager.cpp \
	XMLBlob.cpp \
	XMLDoc.cpp \
	XMLNode.cpp \
	XMLTextNode.cpp 

HEADERS = \
	AnyPtr.h \
	AutoMutex.h \
	AutoDoUndo.h \
	AutoDynamicLibraryHandle.h \
	Base64.h \
	CheckedValue.h \
	CheckedValueStore.h \
	Clock.h \
	Condition.h \
	Curl.h \
	DaemonUtil.h \
	DbAutoConnection.h \
	DbAutoTrans.h \
	DbConnection.h \
	DbMyConnection.h \
	DbPgConnection.h \
	DbLiteConnection.h \
	DbConnectionPool.h \
	DbException.h \
	DbRow.h \
	DbResult.h \
	DbMyResult.h \
	DbPgResult.h \
	DbLiteResult.h \
	DbUtil.h \
	Dispatcher.h \
	Event.h \
	EventQueue.h \
	Exception.h \
	ExpDecayingAvg.h \
	FMD5.h \
	Forte.h \
	FString.h \
	FTime.h \
	FTrace.h \
	GUIDGenerator.h \
	LogManager.h \
	LogTimer.h \
	Murmur.h \
	OpenSSLInitializer.h \
	PDU.h \
	PDUPeer.h \
	PDUPeerSet.h \
	ProcRunner.h \
	ProcessHandle.h \
	ProcessManager.h \
	PropertyObject.h \
	PidFile.h \
	Random.h \
	RandomGenerator.h \
	RequestHandler.h \
	RWLock.h \
	SecureEnvelope.h \
	SecureString.h \
	Semaphore.h \
	ServerMain.h \
	ServiceConfig.h \
	SSHRunner.h \
	Thread.h \
	ThreadKey.h \
	Types.h \
	UrlString.h \
	Util.h \
	VersionManager.h \
	XMLBlob.h \
	XMLDoc.h \
	XMLNode.h \
	XMLTextNode.h

INSTALL_HEADERS = $(HEADERS:%=$(HEADER_INSTALL_PATH)/%)
HEADER_PERM = 644

INSTALL_LIB = $(LIB_INSTALL_PATH)/libforte.a
LIB_PERM = 644

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
LIB = $(TARGETDIR)/libforte.a
LIBDEPS = $(REVISION_H)

TSRCS = \
	UtilTest.cpp
TOBJS = $(TSRCS:%.cpp=$(TARGETDIR)/%.o)
TPROG = $(TARGETDIR)/utiltest
TLIBS = -L$(TARGETDIR) -lforte -lpthread

INSTALL = $(if $(RPM), @install $(1) $< $@, @install $(1) $(2) $< $@)

all: $(LIB) $(TARGETDIR)/procmon
	$(MAKE_SUBDIRS)

$(TARGETDIR)/procmon: $(TARGETDIR)/procmon.o $(TARGETDIR)/ProcessMonitor.o $(LIB)
	$(CCC) -o $@ $< $(TARGETDIR)/ProcessMonitor.o -L$(TARGETDIR) -lforte -lpthread

utiltest: $(TPROG)

$(TPROG): $(LIB) $(TOBJS)
	$(CCC) -o $@ $(TOBJS) $(TLIBS)

install:: $(LIB) $(INSTALL_HEADERS) $(INSTALL_LIB)

$(HEADER_INSTALL_PATH)/%: %
	@echo Installing $@
	$(call INSTALL,-D,-m $(HEADER_PERM))

$(INSTALL_LIB): $(LIB)
	@echo Installing $@
	$(call INSTALL,-D,-m $(LIB_PERM))

include $(BUILDROOT)/re/make/tail.mk

