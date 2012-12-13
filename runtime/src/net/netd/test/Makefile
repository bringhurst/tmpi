SHELL = /bin/tcsh

ifeq ($(origin OPT), undefined)
OPT = -O3 -Wall
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
OBJ_NETD_SOCK = ../netd_sock.o ../netd_op.o ../sock_sr.o ../sock_conn.o
OBJ_NETQ = ../netq.o ../fifo.o ../salf.o $(UTIL)/tmpi_debug.o ../../../ukey/pthread/ukey_pthread.o ../../../machine/atomic.o ../../../machine/cas64.o ../../../thread/pthread/thread.o

TARGET = sr_test bcast reduce reduce2 allreduce bcast2 bcast3 bcast4 sr_test2 sr_crash fifotest satest satest2 mtnetd simple pp_dat pp_msg nqsimple par_sr bcast5 hcube oneway pp_msgdat

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

satest.o : satestcomm.h
satest2.o : satestcomm.h
../netd_sock.o : ../netd_sock_c
	\cp ../netd_sock_c ../netd_sock.c
	$(CC) $(CFLAGS) -c ../netd_sock.c -o $@
	@\rm -f ../netd_sock.c

../salf.o : ../salf_c
	\cp ../salf_c ../salf.c
	$(CC) $(CFLAGS) -c ../salf.c -o $@
	@\rm -f ../salf.c

bcast5 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o bcast5.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast4 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o bcast4.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

hcube : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o hcube.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast3 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o bcast3.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast2 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o bcast2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

bcast : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o bcast.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

sr_test : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o sr_test.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o reduce.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

reduce2 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o reduce2.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

allreduce : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o allreduce.o $(UTIL)/usc/usc_time.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

sr_test2 : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o sr_test2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

sr_crash : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o sr_crash.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

fifotest : $(UTIL)/tmpi_debug.o ../../../ukey/pthread/ukey_pthread.o ../fifo.o ../../../machine/atomic.o ../../../machine/cas64.o ../../../thread/pthread/thread.o fifotest.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB2)

satest : $(UTIL)/tmpi_debug.o ../../../ukey/pthread/ukey_pthread.o ../stackallocator.o ../../../machine/atomic.o ../../../machine/cas64.o ../../../thread/pthread/thread.o satest.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB2)

satest2 : $(UTIL)/tmpi_debug.o ../../../ukey/pthread/ukey_pthread.o ../salf.o ../../../machine/atomic.o ../../../machine/cas64.o ../../../thread/pthread/thread.o satest2.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB2)

mtnetd : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o ../../../machine/atomic.o ../../../machine/cas64.o mtnetd.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB) $(LIB2)

simple : simple.o $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o
	$(CC) $(CFLAGS) $+ -o $@

pp_dat : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o pp_dat.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

pp_msg : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o pp_msg.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

pp_msgdat : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o pp_msgdat.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

nqsimple : nqsimple.o $(OBJ_NETD_SOCK) $(OBJ_NETQ)
	$(CC) $(CFLAGS) $+ -o $@ $(LIB2)

par_sr : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o ../../../machine/atomic.o ../../../machine/cas64.o par_sr.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB) $(LIB2)

oneway : $(OBJ_NETD_SOCK) $(UTIL)/tmpi_debug.o $(UTIL)/usc/usc_time.o oneway.o
	$(CC) $(CFLAGS) $+ -o $@ $(LIB)

