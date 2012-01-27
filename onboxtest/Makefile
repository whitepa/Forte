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
	$(OS_LIBS) \
	$(SSH2_LIBS) \
	$(DB_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_REGEX_LIB) \
	-lpthread -lrt

PROGS=  $(TARGETDIR)/ActiveObjectUnitTest \
	$(TARGETDIR)/DbMirroredConnectionUnitTest \
	$(TARGETDIR)/INotifyUnitTest \
	$(TARGETDIR)/ProcessManagerOnBoxTest \
	$(TARGETDIR)/RunLoopUnitTest \
	$(TARGETDIR)/SCSIUtilUnitTest \
	$(TARGETDIR)/ServerMainOnboxTest

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###
all: $(GTEST) $(PROGS) 

$(PROGS) : $(TARGETDIR)/% : %.cpp
	@echo "Building $@ from $*.cpp ..."
	$(QUIET)$(CCC) $*.cpp \
	-o $@ \
	$(COMMON_INCLUDE) \
	$(COMMON_LIBS) \

install: all 
	@$(MKDIR) $(PREFIX)/opt/scale/lib/qa/onbox
	@echo "Copying files to $(PREFIX)"
	@$(CP) $(PROGS) $(PREFIX)/opt/scale/lib/qa/onbox
	@$(CP) ServerMainOnboxTest.info $(PREFIX)/opt/scale/lib/qa/onbox

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
