# Makefile for forte tests
# Copyright (C) 2008 Alchemy Technology Partners, LLC
# Copyright (C) 2010 Scale Computing, Inc.

# To add a new test, create a new .cpp file in this directory.  It
# will be compiled as a standalone executable.

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

INCLUDE = \
	-I. \
	$(FORTE_INCLUDE) \
	$(DB_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(XML_INCLUDE)

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \
	$(BOOST_LIBS) \
	$(CURL_LIBS) \
	$(OS_LIBS) \
	$(BOOST_UNIT_TEST_LIB_STATIC)

SRCS = $(wildcard *.cpp)

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
PROGS = $(SRCS:%.cpp=$(TARGETDIR)/%)

PROG_DEPS = $(FORTE)

###  RULES  ###

all: $(PROGS)

# progs
$(foreach p,$(subst $(TARGETDIR)/,,$(PROGS)),$(eval $(call GENERATE_LINK_TEST_RULE,$(p))))

# tail
include $(BUILDROOT)/re/make/tail.mk
