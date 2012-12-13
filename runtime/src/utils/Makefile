SHELL = /bin/tcsh

ifeq ($(origin DEPENDFLAG), undefined)
DEPENDFLAG = -MM
endif

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

DIRS = usc buffer tqueue

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS=$(OPT)

.PHONY : default all clean FORCE
default all : build $(DIRS) $(OBJ)
	@touch build
	@echo `date` >>build

clean : $(DIRS)

export 

$(DIRS) : FORCE
	@$(MAKE) -C $@ $(MAKECMDGOALS)

build : Makefile
	@$(MAKE) clean

clean : 
	@- \rm -f *~
	@- \rm -f $(OBJ)

FORCE : ;

$(OBJ) : .depend

.depend : $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(DEPENDFLAG)  *.c >! $@

ifeq (.depend, $(wildcard .depend))
include .depend
endif
