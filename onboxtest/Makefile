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

PROGS=$(TARGETDIR)/SCSIUtilUnitTest

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

install: 
# $(PROGS)
# 	@echo "Creating paths under $(PREFIX)"
# 	@$(MKDIR) $(PREFIX)/opt/scale/lib/qa

# 	@echo "Copying files to $(PREFIX)"
# 	@$(CP) $(PROGS) $(PREFIX)/opt/scale/lib/qa

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all
