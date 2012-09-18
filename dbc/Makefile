
BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
$(make-targetdir)

CFLAGS = -DYYDEBUG=1

INCLUDE = $(FORTE_INCLUDE) -I$(BOOST_INCLUDE)

CCCARGS += -Wno-unused-function

LIBS = $(FORTE_LIBS) $(BOOST_REGEX_LIB) -lfl -lpthread

CLEAN += SQLParse.[ch]pp SQLScan.cpp SQLParse.output $(EXPORT_BIN)/dbc

OBJDEPS=SQLScan.ll SQLParse.yy

PARSER_SRC=SQLParse.cpp SQLScan.cpp
SRCS=   dbc.cpp \
	$(PARSER_SRC) \
	InternalRep.cpp

OBJS=$(SRCS:%.cpp=$(TARGETDIR)/%.o)

PROG=$(TARGETDIR)/dbc
PROG_DEPS = $(PARSER_SRC)

all: $(PROG)

parser: $(PARSER_SRC)

$(eval $(call GENERATE_LINK_PROG_RULE,dbc))

include $(BUILDROOT)/re/make/tail.mk

.PHONY: parser
