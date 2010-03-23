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

INCLUDE = $(DB_INCLUDE) $(XML_INCLUDE) $(BOOST_INCLUDE) -I.
CCARGS += -Wall -DFORTE_FUNCTION_TRACING
SRCS =	\
	AutoMutex.cpp \
	Base64.cpp \
	CheckedValue.cpp \
	ClusterLock.cpp \
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
	Exception.cpp \
	ExpDecayingAvg.cpp \
	FileSystem.cpp \
	FString.cpp \
	FTime.cpp \
	FTrace.cpp \
	GUID.cpp \
	LogManager.cpp \
	LogTimer.cpp \
	MD5.cpp \
	Murmur.cpp \
	OnDemandDispatcher.cpp \
	OpenSSLInitializer.cpp \
	PidFile.cpp \
	ProcRunner.cpp \
	Random.cpp \
	ReceiverThread.cpp \
	RWLock.cpp \
	SecureEnvelope.cpp \
	SecureString.cpp \
	ServerMain.cpp \
	ServiceConfig.cpp \
	Thread.cpp \
	ThreadKey.cpp \
	ThreadPoolDispatcher.cpp \
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
	Base64.h \
	Callback.h \
	Condition.h \
	ControlFile.h \
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
	DelayedCallback.h \
	Dispatcher.h \
	Event.h \
	EventObserver.h \
	EventQueue.h \
	Exception.h \
	ExpDecayingAvg.h \
	FMD5.h \
	Forte.h \
	FString.h \
	FTime.h \
	FTrace.h \
	GUID.h \
	LogManager.h \
	LogTimer.h \
	Murmur.h \
	OpenSSLInitializer.h \
	ProcRunner.h \
	PidFile.h \
	Random.h \
	RequestHandler.h \
	RWLock.h \
	SecureEnvelope.h \
	SecureString.h \
	Semaphore.h \
	ServerMain.h \
	ServiceConfig.h \
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

all: $(LIB)

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

