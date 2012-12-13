SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -O3 -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I. -I$(TMPI_HOME)/include
CFLAGS = $(OPT) $(INC)
LIB = -lpthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
OBJ_TEST = ../barrier.o ../../thread/pthread/thread.o ../../ukey/pthread/ukey_pthread.o ../../utils/tmpi_debug.o ../../machine/atomic.o ../../machine/cas64.o

TARGET = bar_test

.PHONY : default all clean FORCE

default all : build $(TARGET)
	@touch build
	@echo `date` >>build

export 

build : Makefile
	@$(MAKE) clean

clean : FORCE
	@- \rm -f *~
	@- \rm -f $(TARGET) $(OBJ)

FORCE : ;

bar_test : bar_test.o $(OBJ_TEST)
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)
