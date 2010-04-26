# Makefile for forte mock objects library
#
# Copyright (C) 2010. All rights reserved.
#

# set build root
BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)
include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CCARGS += -Wall

INCLUDE = \
	-I. \
	$(FORTE_INCLUDE) \

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \

LIB=$(TARGETDIR)/libfortemockobjects.a

SRCS = \
	MockCurl.cpp \
	MockCurlFactory.cpp \
	MockProcRunner.cpp \
	MockFileSystem.cpp \

OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)

LIBDEPS=$(FORTE)

###  RULES  ###

all: $(LIB)


# tail
include $(BUILDROOT)/re/make/tail.mk
