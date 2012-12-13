/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

#include <stdio.h>
#include <varargs.h>
#include "tmpi_debug.h"

# define OUTPUT stdout

static int debug_level=0;
static int debug_line=-1;
static char *debug_file="<unknown>";

int tmpi_setparam(int line, char *file)
{
	debug_line=line;
	debug_file=file;
	
	return 0;
}

int tmpi_get_dbg_level()
{
    return(debug_level);
}

void tmpi_set_dbg_level(int level)
{
	if (level>DBG_NONE) level=DBG_NONE;
	if (level<=DBG_ALL) level=DBG_ALL+1;
    debug_level = level;
}

int tmpi_error0(va_alist)
va_dcl
{
    va_list ap;
	int level;
	char *fmt;

    va_start(ap);
	level=va_arg(ap, int);
	fmt=va_arg(ap, char *);
    if (level <= debug_level)
		return 0;
    fprintf(OUTPUT, "[%d]:(%s:%d) ", level, debug_file, debug_line);
    vfprintf(OUTPUT, fmt, ap);
    fprintf(OUTPUT, "\n");
    va_end(ap);
    fflush(OUTPUT);

	debug_line=-1;
	debug_file="<unknown>";

	return 0;
}

int tmpi_debug(int level)
{
	if (level <= debug_level)
		return 0;
	return 1;
}

int tmpi_error1(va_alist)
va_dcl
{
    va_list ap;
	int level;
	char *fmt;

    va_start(ap);
	level=va_arg(ap, int);
	debug_line=va_arg(ap, int);
	debug_file=va_arg(ap, char *);
	fmt=va_arg(ap, char *);
    if (level <= debug_level)
		return 0;
    fprintf(OUTPUT, "[%d]:(%s:%d) ", level, debug_file, debug_line);
    vfprintf(OUTPUT, fmt, ap);
    fprintf(OUTPUT, "\n");
    va_end(ap);
    fflush(OUTPUT);

	debug_line=-1;
	debug_file="<unknown>";

	return 0;
}
