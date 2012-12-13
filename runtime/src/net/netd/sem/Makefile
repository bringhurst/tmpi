SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I. -I$(TMPI_HOME)/include
CFLAGS = $(OPT) $(INC)

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY : default all clean FORCE

default all : build $(OBJ)
	@touch build
	@echo `date` >>build

export 

build : Makefile
	@$(MAKE) clean

clean : 
	@- \rm -f *~
	@- \rm -f $(OBJ)

FORCE : ;

sem.o : sem.h
