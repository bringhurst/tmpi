SHELL = /bin/tcsh

DEPENDFLAG = -MM

# I don't want gcc to over optimize the machine code
OPT = -O -Wall

INC = -I. -I..

# lock instructions
DEFS = -D__SMP__
CFLAGS = $(OPT) $(INC) $(DEFS)

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

TARGETS = virus psize tatomic scon scon2 incsub aadd

.PHONY : default all clean FORCE

default all : build $(TARGETS)
	@touch build
	@echo `date` >>build

build : Makefile
	@$(MAKE) clean

virus : virus.o

psize : psize.o

tatomic : tatomic.o ../atomic.o ../cas64.o

scon : scon.o ../atomic.o ../cas64.o
	$(CC) $(CFLAGS) $+ -o $@ -lpthread 

scon2 : scon2.o ../atomic.o ../cas64.o
	$(CC) $(CFLAGS) $+ -o $@ -lpthread 

incsub : incsub.o ../atomic.o ../cas64.o
	$(CC) $(CFLAGS) $+ -o $@ -lpthread 

aadd : aadd.o ../atomic.o ../cas64.o
	$(CC) $(CFLAGS) $+ -o $@ -lpthread 

clean : FORCE
	@- \rm -f *~
	@- \rm -f $(OBJ)
	@- \rm -f $(TARGETS)

FORCE : ;

$(OBJ) : .depend

.depend : $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(DEPENDFLAG)  *.c >! $@

ifeq (.depend, $(wildcard .depend))
include .depend
endif

