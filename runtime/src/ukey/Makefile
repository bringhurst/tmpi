SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime
endif

DIRS = pthread

export

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
