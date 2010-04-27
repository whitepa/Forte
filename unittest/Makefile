# Makefile for unit tests
# Copyright (C) 2008 Alchemy Technology Partners, LLC
# Copyright (C) 2010 Scale Computing, Inc.

# To add a new test, create a new .cpp file in this directory.  It
# will be compiled as a standalone executable.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall -ggdb

INCLUDE = \
	-I. \
	-I.. \
	$(BOOST_INCLUDE) 

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \
	$(BOOST_LIBS) \
	$(BOOST_FS_LIB) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_REGEX_LIB) \
	$(CURL_LIBS) \
	$(OS_LIBS) \
	$(BOOST_UNIT_TEST_LIB_STATIC)

SRCS = $(wildcard *.cpp)

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
PROGS = $(SRCS:%.cpp=$(TARGETDIR)/%)


# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)

###  RULES  ###

all: parent $(PROGS) 

parent: FORCE
	$(MAKE) -C ../

# progs
$(PROGS): $(TARGETDIR)/%: $(FORTE) $(FORTE_MOCK) %.cpp $(TARGETDIR)/%.o
	@echo "Building $@ ..."
	$(CCC) -o $@ $@.o $(LIBS)

# tail
include $(BUILDROOT)/re/make/tail.mk

.PHONY: parent all