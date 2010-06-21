
BUILDROOT:=$(shell echo 'while [ ! -d re ]; do cd ..; done; pwd' | sh)

include $(BUILDROOT)/re/make/head.mk
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

PARSER_SRC=SQLParse.cpp SQLScan.cpp
SRCS=   dbc.cpp \
	$(PARSER_SRC) \
	InternalRep.cpp

OBJS=$(SRCS:%.cpp=$(TARGETDIR)/%.o)

PROG=$(TARGETDIR)/dbc

all: $(PROG)

parser: $(PARSER_SRC)

$(PROG): $(PARSER_SRC) $(OBJS) Makefile $(OBJDEPS)
	@echo Linking $@
	$(CCC) -o $@ $(OBJS) $(LIBS) $(OS_LIBS)
	$(export-bin)

SQLParse.cpp: SQLParse.yy Makefile
	$(YACC) -o $@ $<

SQLScan.cpp: SQLScan.ll Makefile
	$(LEX) -o$@ $<



include $(BUILDROOT)/re/make/tail.mk

.PHONY: parser