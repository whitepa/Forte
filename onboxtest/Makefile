# Copyright (C) 2010 Scale Computing, Inc.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -ggdb -msse4.2
DEFS += -DFORTE_WITH_SQLITE

INCLUDE = \
	-I. \
	-I../mockobjects \
	$(GTEST_INCLUDE) \
	$(FORTE_INCLUDE) \
	$(SSH2_INCLUDE)

LIBS = \
	$(FORTE_DB_SQLITE) \
	$(FORTE_LIBS) \
	$(SSH2_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_REGEX_LIB) \
	$(GTEST_LIB) \
	$(SQLITE_LIBS) \
	$(OS_LIBS)

PROGS=  $(TARGETDIR)/ActiveObjectUnitTest \
	$(TARGETDIR)/CRCOnBoxTest \
	$(TARGETDIR)/ExponentiallyDampedMovingAverageOnBoxTest \
	$(TARGETDIR)/EPollMonitorOnBoxTest \
	$(TARGETDIR)/FileSystemImplOnBoxTest \
	$(TARGETDIR)/INotifyUnitTest \
	$(TARGETDIR)/InterProcessLockOnBoxTest \
	$(TARGETDIR)/PDUPeerSetBuilderImplOnBoxTest \
	$(TARGETDIR)/PDUPeerSetImplOnBoxTest \
	$(TARGETDIR)/ProcessManagerOnBoxTest \
	$(TARGETDIR)/RunLoopUnitTest \
	$(TARGETDIR)/SCSIUtilUnitTest \
	$(TARGETDIR)/ServerMainOnboxTest \
	$(TARGETDIR)/SocketUtilOnBoxTest \
	$(TARGETDIR)/StateMachineOnBoxTest3 \

PROG_DEPS_OBJS_EPollMonitorOnBoxTest = \
	../$(TARGETDIR)/EPollMonitor.o \


PROG_DEPS_OBJS_PDUPeerSetImplOnBoxTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerSetBuilderImpl.o \
	../$(TARGETDIR)/PDUPeerSetConnectionHandler.o \
	../$(TARGETDIR)/PDUPeerEndpointFactoryImpl.o \
	../$(TARGETDIR)/PDUPeerEndpointNetworkConnector.o \
	../$(TARGETDIR)/PDUPeerEndpointInProcess.o \
	../$(TARGETDIR)/PDUPeerSetImpl.o \
	../$(TARGETDIR)/PDUPeerImpl.o \
	../$(TARGETDIR)/PDUPeerEndpointFD.o \
	../$(TARGETDIR)/Thread.o \
	../$(TARGETDIR)/ThreadPoolDispatcher.o \
	../$(TARGETDIR)/OnDemandDispatcher.o \
	../$(TARGETDIR)/ReceiverThread.o \

PROG_DEPS_OBJS_SocketUtilOnBoxTest = \
	../$(TARGETDIR)/SocketUtil.o \

PROG_DEPS_OBJS_PDUPeerSetBuilderImplOnBoxTest = \
	../$(TARGETDIR)/PDU.o \
	../$(TARGETDIR)/PDUPeerSetBuilderImpl.o \
	../$(TARGETDIR)/PDUPeerSetConnectionHandler.o \
	../$(TARGETDIR)/PDUPeerEndpointFactoryImpl.o \
	../$(TARGETDIR)/PDUPeerEndpointNetworkConnector.o \
	../$(TARGETDIR)/PDUPeerEndpointInProcess.o \
	../$(TARGETDIR)/PDUPeerSetImpl.o \
	../$(TARGETDIR)/PDUPeerImpl.o \
	../$(TARGETDIR)/PDUPeerEndpointFD.o \
	../$(TARGETDIR)/Thread.o \
	../$(TARGETDIR)/ThreadPoolDispatcher.o \
	../$(TARGETDIR)/OnDemandDispatcher.o \
	../$(TARGETDIR)/ReceiverThread.o \

###  RULES  ###
all: $(GTEST) $(PROGS)

$(foreach p,$(subst $(TARGETDIR)/,,$(PROGS)),$(eval $(call GENERATE_LINK_TEST_RULE,$(p))))

install: all
	@$(MKDIR) $(PREFIX)/opt/scale/lib/qa/onbox
	@echo "Copying files to $(PREFIX)"
	@$(CP) $(PROGS) $(PREFIX)/opt/scale/lib/qa/onbox
	@$(CP) ServerMainOnboxTest.info $(PREFIX)/opt/scale/lib/qa/onbox
	@$(call GENERATE_ONBOX_TAGS, $(PROGS), $(PREFIX)/opt/scale/lib/qa/onbox)


include $(BUILDROOT)/re/make/tail.mk

