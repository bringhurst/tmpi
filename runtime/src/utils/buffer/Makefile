SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -O2 -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src/
endif

INC = -I. -I$(TMPI_HOME)/include
CFLAGS = $(OPT) $(INC)

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY : default all clean FORCE

default all : build $(OBJ)
	@touch build
	@echo `date` >>build

build : Makefile
	@$(MAKE) clean

clean :
	@- \rm -f *~
	@- \rm -f $(OBJ)

mbuffer.o : mbuffer.h

FORCE : ;
