# Makefile
#
# Copyright 2008 Scale Computing, Inc.
# Proprietary and Confidential
#

BUILDROOT:=$(shell echo 'while [ ! -d buildtools ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/buildtools/common/head.mk
$(make-targetdir)

INCLUDE = -I. -I$(FORTE_DIR) $(PGSQL_INCLUDE) $(MYSQL_INCLUDE) $(SQLITE_INCLUDE) $(XML_INCLUDE)
CCARGS += -Wall
SRCS =	\
	Test.cpp

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
LIBS = $(FORTE_LIBS) $(MYSQL_LIBS) $(PGSQL_LIBS) $(SQLITE_LIBS) $(OS_LIBS)
MYPROG = $(TARGETDIR)/Test

all: $(MYPROG) runtest

.NOTPARALLEL:

$(MYPROG): $(OBJS) FORCE
	@$(CCC) -o $@ $< $(LIBS)
	@if [ ! -f $@ ]; then echo Test failed to compile; exit 1; fi

runtest: FORCE
	@echo Running test
	@$(MYPROG) || /bin/true

include $(BUILDROOT)/buildtools/common/tail.mk

