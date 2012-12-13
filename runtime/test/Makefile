SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
# OPT = -O2 -Wall
OPT = -g -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime
endif

INC = -I. -I$(TMPI_HOME)/include
CFLAGS = $(OPT) $(INC)
LIB = -ltmpi -lpthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

TARGET = gene_test bar_test bcast_test1 bcast_test2 bcast_test3 reduce_test1 reduce_test2 reduce_test3 allreduce cpuid pp pp_dat ow_dat

.PHONY : default all clean FORCE

default all : build $(TARGET)
	@touch build
	@echo `date` >>build

build : Makefile
	@$(MAKE) clean

clean : FORCE
	@- \rm -f *~
	@- \rm -f $(TARGET) $(OBJ)

FORCE : ;

gene_test: gene_test.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bar_test: bar_test.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast_test1: bcast_test1.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast_test2: bcast_test2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast_test3: bcast_test3.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce_test1: reduce_test1.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce_test2: reduce_test2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce_test3: reduce_test3.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

allreduce: allreduce.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

cpuid: cpuid.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

pp : pp.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

pp_dat : pp_dat.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

ow_dat : ow_dat.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)
