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

FORTE_DB_VARIANTS = MYSQL SQLITE MYSQL_SQLITE #PGSQL

define VARIANT_OBJS
OBJS_$(1) = $(DB_SRCS:%.cpp=$(TARGETDIR)/$(1)/%.o)
endef

define VARIANT_COMPILE_RULE
$(TARGETDIR)/$(1)/%.o: %.cpp $(MAKEFILE_DEPS)
	@$(MKDIR) $$(@D)
	$(QUIET)$(call CHECKSUM_TEST_EVAL,$$@,$(PREPROCESS.CCC.EVAL) $(foreach v,$(subst _, ,$(1)),-DFORTE_WITH_$(v)) $(DB_INCLUDE),echo "Compiling $(1) $$<";$(COMPILE.CCC.EVAL)  $(foreach v,$(subst _, ,$(1)),-DFORTE_WITH_$(v)) $(DB_INCLUDE))
endef

define VARIANT_LINK_RULE
$(TARGETDIR)/$(1)/libforte_db.a: $(OBJS_$(1)) $(MAKEFILE_DEPS)
	@echo "Linking $$@: $(OBJS_$(1))"
	$(QUIET)ar rcs $$@ $(OBJS_$(1))
endef

SUBDIRS = dbc mockobjects unittest onboxtest

INCLUDE = -I. $(XML_INCLUDE) $(BOOST_INCLUDE) $(SSH2_INCLUDE)

DEFS += -DFORTE_FUNCTION_TRACING

CLEAN += $(TARGETDIR)/utiltest

SRCS =	\
	ActiveObjectThread.cpp \
	Base64.cpp \
	AdvisoryLock.cpp \
	ClusterLock.cpp \
	CheckedValue.cpp \
	CheckedValueStore.cpp \
	Clock.cpp \
	Collector.cpp \
	Condition.cpp \
	ConditionFileLogger.cpp \
	ConditionAsyncLogger.cpp \
	ConditionDeltaLogger.cpp \
	ConditionLogEntry.cpp \
	Context.cpp \
	ContextImpl.cpp \
	ContextPredicate.cpp \
	Curl.cpp \
	DaemonUtil.cpp \
	Dispatcher.cpp \
	EventQueue.cpp \
	EventPredicate.cpp \
	Exception.cpp \
	ExpDecayingAvg.cpp \
	FileSystemImpl.cpp \
	FileSystemUtil.cpp \
	FString.cpp \
	FTime.cpp \
	FTrace.cpp \
	GUIDGenerator.cpp \
	INotify.cpp \
	LogManager.cpp \
	LogTimer.cpp \
	MD5.cpp \
	Murmur.cpp \
	OnDemandDispatcher.cpp \
	OpenSSLInitializer.cpp \
	PDU.cpp \
	PDUPeerEndpointFactoryImpl.cpp \
	PDUPeerFileDescriptorEndpoint.cpp \
	PDUPeerImpl.cpp \
	PDUPeerInProcessEndpoint.cpp \
	PDUPeerNetworkAcceptorEndpoint.cpp \
	PDUPeerNetworkConnectorEndpoint.cpp \
	PDUPeerSetBuilderImpl.cpp \
	PDUPeerSetConnectionHandler.cpp \
	PDUPeerSetWorkHandler.cpp \
	PDUPeerSetImpl.cpp \
	PidFile.cpp \
	PosixTimer.cpp \
	ProcFileSystem.cpp \
	ProcRunner.cpp \
	ProcessFutureImpl.cpp \
	ProcessManagerImpl.cpp \
	ProcessorInformation.cpp \
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
	StateHistoryLogger.cpp \
	StateMachine.cpp \
	StateMachineTestHarness.cpp \
	StateRegion.cpp \
	Thread.cpp \
	ThreadKey.cpp \
	ThreadPoolDispatcher.cpp \
	ThreadSafeObjectMap.cpp \
	Timer.cpp \
	UrlString.cpp \
	Util.cpp \
	VersionManager.cpp \
	XMLBlob.cpp \
	XMLDoc.cpp \
	XMLInitializer.cpp \
	XMLNode.cpp \
	XMLTextNode.cpp

DB_SRCS = \
	DbConnection.cpp \
	DbConnectionPool.cpp \
	DbMyConnection.cpp \
	DbPgConnection.cpp \
	DbLiteConnection.cpp \
	DbLiteConnectionFactory.cpp \
	DbMirroredConnection.cpp \
	DbMirroredConnectionFactory.cpp \
	DbSqlStatement.cpp \
	DbBackupManager.cpp \
	DbBackupManagerThread.cpp \
	DbException.cpp \
	DbRow.cpp \
	DbResult.cpp \
	DbMyResult.cpp \
	DbPgResult.cpp \
	DbLiteResult.cpp \
	$(NULL)

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
	DbLiteConnectionFactory.h \
	DbMirroredConnection.h \
	DbMirroredConnectionFactory.h \
	DbBackupManager.h \
	DbSqlStatement.h \
	DbBackupManagerThread.h \
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
	INotify.h \
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
	XMLInitializer.h \
	XMLTextNode.h

INSTALL_HEADERS = $(HEADERS:%=$(HEADER_INSTALL_PATH)/%)
HEADER_PERM = 644

INSTALL_LIB = $(LIB_INSTALL_PATH)/libforte.a
LIB_PERM = 644

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
LIB = $(TARGETDIR)/libforte.a

TPROGS = UtilTest procmon
PROGS = $(addprefix $(TARGETDIR)/,$(TPROGS))

PROG_DEPS = Makefile
PROG_DEPS_OBJS_procmon = $(TARGETDIR)/ProcessMonitor.o
LIBS_procmon = $(FORTE_LIBS) $(OS_LIBS) $(BOOST_REGEX_LIB)
LIBS_UtilTest = $(FORTE_LIBS) $(OS_LIBS)
PROG_DEPS_procmon = $(LIB)
PROG_DEPS_UtilTest = $(LIB)

INSTALL = $(if $(RPM), @install $(1) $< $@, @install $(1) $(2) $< $@)

$(foreach v,$(FORTE_DB_VARIANTS),$(eval $(call VARIANT_OBJS,$v)))

LIB_DB_VARIANTS = $(foreach v,$(FORTE_DB_VARIANTS),$(TARGETDIR)/${v}/libforte_db.a)

all: $(LIB) $(LIB_DB_VARIANTS) $(TARGETDIR)/procmon
	$(MAKE_SUBDIRS)

$(foreach p,$(TPROGS),$(eval $(call GENERATE_LINK_TEST_RULE,$(p))))

utiltest: $(TARGETDIR)/UtilTest
	ln -f $< $(TARGETDIR)/utiltest

install:: $(LIB) $(INSTALL_HEADERS) $(INSTALL_LIB)

$(HEADER_INSTALL_PATH)/%: %
	@echo Installing $@
	$(call INSTALL,-D,-m $(HEADER_PERM))

$(INSTALL_LIB): $(LIB)
	@echo Installing $@
	$(call INSTALL,-D,-m $(LIB_PERM))

$(foreach v,$(FORTE_DB_VARIANTS),$(eval $(call VARIANT_COMPILE_RULE,$v)))

$(foreach v,$(FORTE_DB_VARIANTS),$(eval $(call VARIANT_LINK_RULE,$v)))

include $(BUILDROOT)/re/make/tail.mk

