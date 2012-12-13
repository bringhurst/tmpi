SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I.
CFLAGS = $(OPT) $(INC)

# cannot make several conflicting thread systems at the same time
DIRS = pthread pthread_lib

.PHONY : default all clean FORCE

default all : build $(DIRS)
	@touch build
	@echo `date` >>build

clean : $(DIRS)

build : Makefile
	@$(MAKE) clean

$(DIRS) : FORCE
	@$(MAKE) -C $@ $(MAKECMDGOALS)

clean :
	@- \rm -f *~

FORCE : ;
