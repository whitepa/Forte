# Makefile for forte tests
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
	$(FORTE_INCLUDE) \
	$(DB_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(XML_INCLUDE)

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \
	$(DB_LIBS) \
	$(BOOST_LIBS) \
	$(CURL_LIBS) \
	$(OS_LIBS) \
	$(BOOST_UNIT_TEST_LIB_STATIC)

SRCS = $(wildcard *.cpp)

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
PROGS = $(SRCS:%.cpp=$(TARGETDIR)/%)

# define LIBDIRS for distclean
LIBDIRS=$(FORTE2_DIR)

###  RULES  ###

all: $(PROGS)

# progs
$(PROGS): $(FORTE2) $(OBJS)
	$(CCC) -o $@ $@.o $(LIBS)

# tail
include $(BUILDROOT)/re/make/tail.mk
