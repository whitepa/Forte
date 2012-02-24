# Copyright (C) 2010 Scale Computing, Inc.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -ggdb
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
	$(OS_LIBS) \
	$(SSH2_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_REGEX_LIB) \
	$(GTEST_LIB) \
	$(SQLITE_LIBS) \
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
	$(QUIET)$(CCC) $(CCCARGS) $*.cpp \
	-o $@ \
	$(LIBS) \

install: all
	@$(MKDIR) $(PREFIX)/opt/scale/lib/qa/onbox
	@echo "Copying files to $(PREFIX)"
	@$(CP) $(PROGS) $(PREFIX)/opt/scale/lib/qa/onbox
	@$(CP) ServerMainOnboxTest.info $(PREFIX)/opt/scale/lib/qa/onbox

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
