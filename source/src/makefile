PROFILE:=0
DEBUG?=0
CC=gcc
CXX:=g++
LD=g++

ifeq ($(PROFILE), 1)
    PGFLAGS:=-pg
else
    PGFLAGS:=
endif

ifeq ($(DEBUG), 1)
    DBGFLAGS:=-g
    OPTFLAGS:=
else
    DBGFLAGS:=
    OPTFLAGS:=-O3 -march=native
endif

# CUDD vars
CUDDROOT=../cudd-2.5.0/
CUDDINCLUDE=-I${CUDDROOT}/include/
CUDDLIBS=$(CUDDROOT)cudd/libcudd.a $(CUDDROOT)util/libutil.a  $(CUDDROOT)obj/libobj.a $(CUDDROOT)/epd/libepd.a $(CUDDROOT)mtr/libmtr.a $(CUDDROOT)st/libst.a $(CUDDROOT)dddmp/libdddmp.a
CUDDLIBRARIES=-L$(CUDDROOT)cudd/ -L$(CUDDROOT)util/  -L$(CUDDROOT)obj/ -L$(CUDDROOT)/epd/ -L$(CUDDROOT)mtr/ -L$(CUDDROOT)st/ -L$(CUDDROOT)dddmp/
CUDDLIBFLAGS=-lobj -ldddmp -lcudd -lmtr -lst -lutil -lepd

# minisat vars
MINISATROOT=../minisat/
MINISATLIB=${MINISATROOT}/core/lib_release.a
MINISATINCLUDE=-I${MINISATROOT}

# cmsat vars
CMSATROOT=../cmsat-2.9.9/
CMSATLIB=$(CMSATROOT)/build/cmsat/.libs/libcryptominisat.a
CMSATLIBRARIES=-L$(CMSATROOT)/build/cmsat/.libs/
CMSATINCLUDE=-I${CMSATROOT} # -I${CMSATROOT}/build
CMSATLIBFLAGS=-lcryptominisat
CMSATSO=$(CMSATROOT)/build/cmsat/.libs/libcryptominisat.so $(CMSATROOT)/build/cmsat/.libs/libcryptominisat-2.9.9.so

# lgl vars.
LGLROOT=../lingeling/
LGLLIB=$(LGLROOT)/liblgl.a
LGLLIBRARIES=-L$(LGLROOT)
LGLINCLUDE=-I${LGLROOT}
LGLLIBFLAGS=-llgl

INCLUDE=${MINISATINCLUDE} ${CPLEXINCLUDE} ${CUDDINCLUDE} ${CMSATINCLUDE} ${LGLINCLUDE}
LIBS=-lfl -lz -lgomp ${LGLLIBFLAGS}
DEFINES=-D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -D IL_STD # -D HAVE_CONFIG_H

# CPLEX defines.
CPLEXINCLUDE:=-I/opt/ibm/ILOG/CPLEX_Studio125/cplex/include/ -I/opt/ibm/ILOG/CPLEX_Studio125/concert/include/
CPLEXLIBRARIES:=-L/opt/ibm/ILOG/CPLEX_Studio125/concert/lib/x86-64_sles10_4.1/static_pic/ -L/opt/ibm/ILOG/CPLEX_Studio125/cplex/lib/x86-64_sles10_4.1/static_pic/
CPLEXLIBFLAGS:=-lilocplex -lcplex -lconcert -lm -lpthread

CXXFLAGS:=-c -fopenmp -std=c++0x ${INCLUDE} ${DEFINES} ${DBGFLAGS} ${OPTFLAGS} ${PGFLAGS} -Wno-strict-overflow # -Wno-unused-result -Wno-unused-function
LFLAGS:=-c -Wall ${INCLUDE} ${DEFINES} ${DBGFLAGS} ${OPTFLAGS} ${PGFLAGS} # -Wno-unused-result -Wno-unused-function
LDFLAGS:=${DBGFLAGS} ${OPTFLAGS} ${PGFLAGS} $(CUDDLIBRARIES)  ${CPLEXLIBRARIES}  $(CMSATLIBRARIES) ${LGLLIBRARIES}
SUFFIXES += .d

#Find all the C++ files in the current directory.
SOURCES:=$(shell find . -name "*.cpp" | grep -v mcqd.cpp)
#These are the dependency files, which make will clean up after it creates them
DEPFILES:=$(patsubst %.cpp,%.d,$(SOURCES))
#Objects
OBJECTS:=$(patsubst %.cpp,%.o,$(SOURCES))

all:sld sle lcmp lcheck lle simplify

lle: sld
	rm -f lle
	ln -s sld lle

sle: sld
	rm -f sle
	ln -s sld sle

lcmp: sld
	rm -f lcmp
	ln -s sld lcmp

lcheck: sld
	rm -f lcheck
	ln -s sld lcheck

simplify: sld
	rm -f simplify
	ln -s sld simplify

sld: lex.yy.o bench.tab.o ${MINISATLIB} ${CMSATLIB} ${LGLLIB} ${CUDDLIBS} $(OBJECTS) 
	$(LD) $(LDFLAGS) -o sld $(OBJECTS) ${CMSATLIB} lex.yy.o bench.tab.o ${MINISATLIB} ${CPLEXLIBFLAGS} ${CUDDLIBFLAGS} ${LIBS}  

lex.yy.o: lex.yy.cc bench.tab.cc
	$(CXX) $(LFLAGS) lex.yy.cc

bench.tab.o: bench.tab.cc ast.h
	$(CXX) $(CXXFLAGS) bench.tab.cc

lex.yy.cc: bench.l 
	flex -o lex.yy.cc bench.l

bench.tab.cc: bench.y ast.h
	bison -d -Wall -rall -o bench.tab.cc bench.y

clean:
	rm -f sld sle lcmp *.o *.cc *.hh *.output *.d

# This is the rule for creating the dependency files
%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MM $< > $@

# This include statement pulls in the commands to build the .o files.
include $(subst .cpp,.d,$(SOURCES))
