SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I.
CFLAGS = $(OPT) $(INC)

PTHREAD_LIB_FILE = /usr/lib/libpthread.a

# cannot make several conflicting thread systems at the same time

.PHONY : default all clean FORCE

default all : build
	@touch build
	@echo `date` >>build

build : Makefile $(PTHREAD_LIB_FILE)
	@$(MAKE) clean
	ar xv $(PTHREAD_LIB_FILE)

clean :

FORCE : ;
