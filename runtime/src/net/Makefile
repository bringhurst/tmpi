SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime 
endif

DIRS = netd

.PHONY : default all clean FORCE

# default all : build p4 $(DIRS)
default all : build $(DIRS)
	@touch build
	@echo `date` >>build

p4 : FORCE
	cd p4; ./setoptflag.pl $(OPT); make P4ARCH=LINUX

$(DIRS) : FORCE
	@$(MAKE) -C $@ $(MAKECMDGOALS)


build : Makefile
	$(MAKE) clean

clean : $(DIRS)
# cd p4; make clean
	@- \rm -f *~

FORCE : ;
