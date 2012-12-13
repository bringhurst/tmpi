SHELL = /bin/tcsh

include Makefile.common

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

DIRS = src
RANLIB = ranlib

SRC_LIST_INTRA = $(wildcard src/intra/*.c)
SRC_LIST_MACHINE = $(wildcard src/machine/*.c)
SRC_LIST_MACHINE_S = $(wildcard src/machine/*.s)
SRC_LIST_MPI = $(wildcard src/mpi/buffer/*.c src/mpi/context/*.c src/mpi/group/*.c src/mpi/init/*.c src/mpi/pt2pt/*.c src/mpi/util/*.c)
SRC_LIST_NET = $(wildcard src/net/netd/*.c)
SRC_LIST_THREAD = $(wildcard src/thread/pthread/*.c)
SRC_LIST_UKEY = $(wildcard src/ukey/pthread/*.c)
SRC_LIST_UTILS = $(wildcard src/utils/*.c src/utils/buffer/*.c src/utils/tqueue/*.c src/utils/usc/*.c)

SRC_LIST = $(SRC_LIST_INTRA) $(SRC_LIST_MACHINE) $(SRC_LIST_MPI) $(SRC_LIST_NET) $(SRC_LIST_THREAD) $(SRC_LIST_UKEY) $(SRC_LIST_UTILS)

# OBJ_LIST = $(SRC_LIST:.c=.o) $(wildcard src/thread/pthread_lib/*.o) $(SRC_LIST_MACHINE_S:.s=.o)
OBJ_LIST = $(SRC_LIST:.c=.o)  $(SRC_LIST_MACHINE_S:.s=.o)

.PHONY : default all clean FORCE

default all : build $(DIRS)
	@$(MAKE) lib
	@touch build
	@echo `date` >>build

clean : $(DIRS)
	@- \rm -f *~

realclean : 
	@- \rm -f $(TARGET_LIB)

build : Makefile.common Makefile
	@$(MAKE) clean

export

$(DIRS) : FORCE
	@$(MAKE) -C $@ $(MAKECMDGOALS)

lib : $(TARGET_LIB)

$(TARGET_LIB_STATIC) : Makefile $(OBJ_LIST)
	$(AR) rcv $@ $(OBJ_LIST)
	$(RANLIB) $@

$(TARGET_LIB_DYNAMIC) : Makefile $(OBJ_LIST)
	$(LD) -G -o $@ $(OBJ_LIST) -lpthread

FORCE : ;
