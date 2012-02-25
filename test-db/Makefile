# Makefile
#
# Copyright 2008 Scale Computing, Inc.
# Proprietary and Confidential
#

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

DEFS += -DFORTE_WITH_MYSQL -DFORTE_WITH_SQLITE

INCLUDE = -I. -I$(FORTE_DIR) $(MYSQL_INCLUDE) $(SQLITE_INCLUDE) $(XML_INCLUDE)

SRCS =	\
	Test.cpp

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)
LIBS = $(FORTE_DB_MYSQL_SQLITE) $(FORTE_LIBS) $(MYSQL_LIBS) $(SQLITE_LIBS) $(OS_LIBS)
MYPROG = $(TARGETDIR)/Test

all: $(MYPROG) runtest

$(eval $(call GENERATE_LINK_PROG_RULE,Test))

runtest: FORCE
	@echo Running test
	@$(MYPROG) || /bin/true

include $(BUILDROOT)/re/make/tail.mk

.NOTPARALLEL:
