/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

# ifndef _TMPI_DEBUG_H_
# define _TMPI_DEBUG_H_

/* print all debug messages */
# define DBG_ALL 0
/* print none of the debug messages */
# define DBG_NONE 10000
/** detailed error messages **/
# define DBG_VERBOSE 1
/** system fatal error, e.g. memory, socket, etc **/
# define DBG_FATAL 1000
/** internal error, something might be a result of programming logic error **/
# define DBG_INTERNAL 500
/** application caller error, less fatal than internal error **/
# define DBG_CALLER 400

/** warnings **/
# define DBG_APP 350
# define DBG_WARN1 300
# define DBG_WARN2 290

/* very detailed information */
# define DBG_INFO 50

# if defined(__GNUC__)
# define tmpi_error(l, fmt...) tmpi_error1(l, __LINE__, __FILE__, ## fmt)
# else
# define tmpi_error tmpi_setparam(__LINE__, __FILE__) || tmpi_error0
# endif

int tmpi_get_dbg_level(void);
void tmpi_set_dbg_level(int level);
int tmpi_error0();
int tmpi_error1();
int tmpi_setparam(int line, char *file);
int tmpi_debug(int level);

# define ASSERT(s) { if (!(s)) tmpi_error(DBG_INTERNAL, "assertion failed\n"); }

# endif
