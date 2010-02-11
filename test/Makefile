# Makefile for forte test

# Copyright (C) 2008 Alchemy Technology Partners, LLC
#

BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

ifdef DEBUG
CCARGS += -DDEBUG_LOG_ON
endif
CCARGS += -Wall -ggdb

INCLUDE = \
	-I. \
	$(FORTE_INCLUDE) \
	$(DB_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(XML_INCLUDE)

#ifeq ($(OS), Darwin)
#MYSQL_LIBS = -L/usr/local/mysql/lib
#FINK_LIBS = -L/sw/lib
#else
#MYSQL_LIBS = -L/usr/lib/mysql
#START_GROUP = -Wl,--start-group
#END_GROUP = -Wl,--end-group
#endif

LIBS = \
	-L$(TARGETDIR) \
	$(FORTE_LIBS) \
	$(DB_LIBS) \
	$(BOOST_LIBS) \
	$(CURL_LIBS) \
	$(OS_LIBS)

PROG = $(TARGETDIR)/fortetest
SRCS = \
	test_string.cpp
OBJS = $(SRCS:%.cpp=$(TARGETDIR)/%.o)

PROG2 = $(TARGETDIR)/filesystem-test
SRCS2 = \
	filesystem.cpp
OBJS2 = $(SRCS2:%.cpp=$(TARGETDIR)/%.o)

PROG3 = $(TARGETDIR)/murmur-test
SRCS3 = \
	murmur.cpp
OBJS3 = $(SRCS3:%.cpp=$(TARGETDIR)/%.o)

PROG4 = $(TARGETDIR)/clusterlock-test
SRCS4 = \
	clusterlock.cpp
OBJS4 = $(SRCS4:%.cpp=$(TARGETDIR)/%.o)

PROG5 = $(TARGETDIR)/xml-test
SRCS5 = \
	xml.cpp
OBJS5 = $(SRCS5:%.cpp=$(TARGETDIR)/%.o)

PROG6 = $(TARGETDIR)/serviceconfig-test
SRCS6 = \
	serviceconfig.cpp
OBJS6 = $(SRCS6:%.cpp=$(TARGETDIR)/%.o)

PROG7 = $(TARGETDIR)/procrunner-test
SRCS7 = \
	procrunner.cpp
OBJS7 = $(SRCS7:%.cpp=$(TARGETDIR)/%.o)

PROG8 = $(TARGETDIR)/curl-test
SRCS8 = \
	curl.cpp
OBJS8 = $(SRCS8:%.cpp=$(TARGETDIR)/%.o)

# define LIBDIRS for distclean
LIBDIRS=$(FORTE_DIR)


###  RULES  ###

all: $(PROG) $(PROG2) $(PROG3) $(PROG4) $(PROG5) $(PROG6) $(PROG7) $(PROG8)

# progs
$(PROG): $(FORTE) $(OBJS)
	$(CCC) -o $@ $(OBJS) $(LIBS)

$(PROG2): $(FORTE) $(OBJS2)
	$(CCC) -o $@ $(OBJS2) $(LIBS)

$(PROG3): $(FORTE) $(OBJS3)
	$(CCC) -o $@ $(OBJS3) $(LIBS)

$(PROG4): $(FORTE) $(OBJS4)
	$(CCC) -o $@ $(OBJS4) $(LIBS)

$(PROG5): $(FORTE) $(OBJS5)
	$(CCC) -o $@ $(OBJS5) $(LIBS)

$(PROG6): $(FORTE) $(OBJS6)
	$(CCC) -o $@ $(OBJS6) $(LIBS)

$(PROG7): $(FORTE) $(OBJS7)
	$(CCC) -o $@ $(OBJS7) $(LIBS)

$(PROG8): $(FORTE) $(OBJS8)
	$(CCC) -o $@ $(OBJS8) $(LIBS)

# tail
include $(BUILDROOT)/re/make/tail.mk
