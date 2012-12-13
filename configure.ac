AC_INIT([tmpi],[0.98a],[github-hpc@lanl.gov],[tmpi],[http://github.com/hpc/tmpi])

# Keep artifacts in directories as much as possible.
AC_CONFIG_AUX_DIR(auxdir)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall no-define])
AC_CONFIG_HEADERS([config.h])

# Check for compilers.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Pull in libtool. We need to support ancient autotools, so don't use LT_INIT.
#LT_INIT
AC_PROG_LIBTOOL

# Checks for typedefs, structs, and compiler options.
AC_TYPE_SIZE_T
AC_C_INLINE

# Checks for library functs.
AC_FUNC_FORK
AC_FUNC_MALLOC
ACX_PTHREAD
AC_CHECK_FUNCS([bzero \
                getcwd \
                gethostbyaddr \
                gethostbyname \
                gethostname \
                gettimeofday \
                inet_ntoa \
                memset \
                realpath \
                socket \
                strchr \
                strcspn \
                strstr \
                strtol])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h \
                  limits.h \
                  netdb.h \
                  netinet/in.h \
                  stdlib.h \
                  string.h \
                  strings.h \
                  sys/param.h \
                  sys/socket.h \
                  sys/time.h \
                  unistd.h])

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src/compile/Makefile \
                 src/compile/mpitrans/Makefile])
#                 src/runtime/Makefile
#                 src/runtime/src/Makefile
#                 src/runtime/src/intra/Makefile
#                 src/runtime/src/intra/test/Makefile
#                 src/runtime/src/machine/Makefile
#                 src/runtime/src/machine/test/Makefile
#                 src/runtime/src/mpi/Makefile
#                 src/runtime/src/mpi/buffer/Makefile
#                 src/runtime/src/mpi/context/Makefile
#                 src/runtime/src/mpi/group/Makefile
#                 src/runtime/src/mpi/init/Makefile
#                 src/runtime/src/mpi/pt2pt/Makefile
#                 src/runtime/src/mpi/util/Makefile
#                 src/runtime/src/net/Makefile
#                 src/runtime/src/net/netd/Makefile
#                 src/runtime/src/net/netd/sem/Makefile
#                 src/runtime/src/net/netd/test/Makefile
#                 src/runtime/src/thread/Makefile
#                 src/runtime/src/thread/pthread/Makefile
#                 src/runtime/src/thread/pthread_lib/Makefile
#                 src/runtime/src/thread/test/Makefile
#                 src/runtime/src/ukey/Makefile
#                 src/runtime/src/ukey/pthread/Makefile
#                 src/runtime/src/utils/Makefile
#                 src/runtime/src/utils/buffer/Makefile
#                 src/runtime/src/utils/tqueue/Makefile
#                 src/runtime/src/utils/usc/Makefile
#                 src/runtime/test/Makefile])
AC_OUTPUT
