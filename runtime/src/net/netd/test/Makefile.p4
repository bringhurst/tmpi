SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -O2 -Wall
endif

ifeq ($(origin TMPI_HOME), undefined)
TMPI_HOME = $(HOME)/mpi/tmpi/runtime/src
endif

INC = -I. -I../../p4/include -I$(TMPI_HOME)/include
UTIL = $(TMPI_HOME)/utils
CFLAGS = $(OPT) $(INC)
LIB = -L$(TMPI_HOME)/net/p4/lib -lp4
LIB2 = -lpthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
OBJ_NETD_SOCK = ../netd_sock.o ../netd_op.o ../sock_sr.o ../sock_conn.o

TARGET = sr_test_p4 bcast_p4 bcast3_p4 bcast4_p4 reduce_p4 reduce2_p4 allreduce_p4 bcast2_p4 sr_test2_p4 mtnetd_p4 simple_p4 pp_dat_p4 pp_msg_p4 bcast5_p4 oneway_p4

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

../netd_p4.o : ../netd_p4_c
	\cp ../netd_p4_c ../netd_p4.c
	$(CC) $(CFLAGS) -c ../netd_p4.c -o $@
	\rm -f ../netd_p4.c

bcast5_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o bcast5.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast4_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o bcast4.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast3_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o bcast3.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast2_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o bcast2.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o bcast.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

sr_test_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o sr_test.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o reduce.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce2_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o reduce2.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

allreduce_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o allreduce.o $(UTIL)/usc/usc_time.o 
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

sr_test2_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o sr_test2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

mtnetd_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o ../../../machine/atomic.o ../../../machine/cas64.o mtnetd.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB) $(LIB2)

simple_p4 : simple.o $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o
	$(CC) $(CFLAGS) $+ -o $@

pp_dat_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o pp_dat.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

pp_msg_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o pp_msg.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

oneway_p4 : ../netd_p4.o $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o oneway.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

