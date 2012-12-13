SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I. -I$(TMPI_HOME)/include
UTIL = $(TMPI_HOME)/utils
CFLAGS = $(OPT) $(INC)
LIB2 = -lpthread
LIB =

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

TARGET = thr_test

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

thr_test : $(UTIL)/tmpi_debug.o ../../ukey/pthread/ukey_pthread.o ../../machine/atomic.o ../../machine/cas64.o ../pthread/thread.o thr_test.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB2)
