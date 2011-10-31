# Copyright (C) 2010 Scale Computing, Inc.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall -ggdb

COMMON_INCLUDE = \
	-I. \
	-I.. \
	-I../mockobjects \
	$(GTEST_INCLUDE) \
	$(FORTE_INCLUDE) \
	$(SSH2_INCLUDE)

COMMON_LIBS = \
	$(GTEST_LIB) \
	$(FORTE_LIBS) \
	$(LIBSCALE_LIBS) \
	$(OS_LIBS) \
	$(SSH2_LIBS) \
	-lpthread -lrt

PROGS=$(TARGETDIR)/SCSIUtilUnitTest $(TARGETDIR)/DbMirroredConnectionUnitTest $(TARGETDIR)/INotifyUnitTest

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###

all: $(GTEST) $(PROGS) 

$(TARGETDIR)/SCSIUtilUnitTest: SCSIUtilUnitTest.cpp
	echo "Building $@..."
	$(CCC) SCSIUtilUnitTest.cpp \
	-o $(TARGETDIR)/SCSIUtilUnitTest \
	$(SCALE_SERVICE_CLIENT_LIBS)  \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS)
	
$(TARGETDIR)/DbMirroredConnectionUnitTest: DbMirroredConnectionUnitTest.cpp $(FORTE)
	@echo "Building $@..."
	$(QUIET)$(CCC) DbMirroredConnectionUnitTest.cpp \
	-o $(TARGETDIR)/DbMirroredConnectionUnitTest \
	$(SCALE_SERVICE_CLIENT_LIBS) \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS) \
	$(BOOST_FS_LIB) \
	$(DB_LIBS) \
	-lsqlite3
	
$(TARGETDIR)/INotifyUnitTest: INotifyUnitTest.cpp $(FORTE)
	@echo "Building $@..."
	$(QUIET)$(CCC) INotifyUnitTest.cpp \
	-o $(TARGETDIR)/INotifyUnitTest \
	$(SCALE_SERVICE_CLIENT_LIBS) \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS) \
	$(BOOST_FS_LIB)

install: all 
	@echo "Creating paths under $(PREFIX)"
	@$(MKDIR) $(PREFIX)/opt/scale/lib/qa/onbox
	@echo "Copying files to $(PREFIX)"
	@$(CP) $(TARGETDIR)/DbMirroredConnectionUnitTest $(PREFIX)/opt/scale/lib/qa/onbox
	@$(CP) $(TARGETDIR)/INotifyUnitTest $(PREFIX)/opt/scale/lib/qa/onbox

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
