
BUILDROOT:=$(shell echo 'while [ ! -d buildtools ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/buildtools/common/head.mk
$(make-targetdir)

LEX = flex
YACC = bison -d -v --report=all
CFLAGS = -DYYDEBUG=1

INCLUDES = $(FORTE_INCLUDE) -I$(BOOST_INCLUDE)
DEFS = -DFORTE_NO_POSTGRESQL -DFORTE_NO_MYSQL

CCARGS += $(INCLUDES) $(DEFS)
CCCARGS = $(CCARGS)

LIBS = $(FORTE_LIBS) -lfl -lpthread

CLEAN += SQLParse.[ch]pp SQLScan.cpp SQLParse.output

OBJDEPS=SQLScan.ll SQLParse.yy

SRCS=   dbc.cpp \
	SQLScan.cpp \
	SQLParse.cpp \
	InternalRep.cpp

OBJS=$(SRCS:%.cpp=$(TARGETDIR)/%.o)

PROG=$(TARGETDIR)/dbc

all: $(PROG)

parser: SQLParse.cpp SQLScan.cpp

$(PROG): ccargs_echo parser $(FORTE) $(OBJS) Makefile $(OBJDEPS)
	$(CCC) -o $@ $(OBJS) $(LIBS) $(OS_LIBS)
	$(export-bin)

SQLParse.cpp: SQLParse.yy Makefile
	$(YACC) -o $@ $<

SQLScan.cpp: SQLScan.ll Makefile
	$(LEX) -o$@ $<



include $(BUILDROOT)/buildtools/common/tail.mk

