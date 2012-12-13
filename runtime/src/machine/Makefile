SHELL = /bin/tcsh

ifeq ($(origin DEPENDFLAG), undefined)
DEPENDFLAG = -MM
endif

# ifeq ($(origin OPT), undefined)
# OPT = -O -Wall
# endif

# I don't want gcc to over optimize the machine code
OPT = -O -Wall

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime
endif

INC = -I.

# lock instructions
DEFS = -D__SMP__
CFLAGS = $(OPT) $(INC) $(DEFS)

CSRC = $(wildcard *.c)
SSRC = $(wildcard *.s)
OBJ = $(CSRC:.c=.o) $(SSRC:.s=.o)

.PHONY : default all clean FORCE

.s.o :

default all : build $(OBJ)
	@touch build
	@echo `date` >>build

$(OBJ) : .depend

.depend: $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(DEPENDFLAG)  *.c >! $@

build : Makefile
	@$(MAKE) clean

clean :
	@- \rm -f *~
	@- \rm -f $(OBJ)

FORCE : ;

ifeq (.depend, $(wildcard .depend))
include .depend
endif
