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

INC = -I. -I$(TMPI_HOME)/include
CFLAGS = $(OPT) $(INC)

VPATH = $(TMPI_HOME)/include

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY : default all clean

default all : build $(SRC:.c=.o)
	@touch build
	@echo `date` >>build

build : Makefile
	@$(MAKE) clean


clean :
	@- \rm -f *~
	@- \rm -f $(OBJ)

$(OBJ) : .depend

.depend : $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(DEPENDFLAG)  *.c >! $@

ifeq (.depend, $(wildcard .depend))
include .depend
endif
