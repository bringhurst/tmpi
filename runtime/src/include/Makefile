SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -O
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

.PHONY : default all clean FORCE

default all clean : FORCE
	@- \rm -f *~
	@touch build
	@echo `date` >>build

build : Makefile
	@$(MAKE) clean

FORCE : ;
