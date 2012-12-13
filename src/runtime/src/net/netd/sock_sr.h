/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

# ifndef _SOCK_SR_H_
# define _SOCK_SR_H_
# include <sys/uio.h>

/**
 * the following functions return 0 on success and -1 on error 
 * It treats the data as an integral whole. And it only succeeds
 * when you completely read/write exactly len bytes of data.
 */

int sock_write(int fd, char *data, int len);
int	sock_writev(int fd, struct iovec *iov, int count);
int	sock_read(int fd, char *data, int len);

/**
 * The following return the actual bytes read by the read operation,
 * up to len bytes.
 */

/* Read until either the buffer is full or the peer closes the connection */
int	sock_read_till_close(int fd, char *data, int len);

/* Read once, will not block if there is data available */
/* returns the length of the actual data received, up to len  */
int sock_readmax(int fd, char *data, int len);

/** 
 * the following return 1 for "yes" or 0 for "no", and "-1" for error 
 * Note since "error" also implies the subsequent read/write will not 
 * block, so even pepole don't check error return, they will discover
 * so afterwards.
 */

int sock_canread(int fd);
int sock_canwrite(int fd);

/** 
 * Socket version of fgets.
 */
char *sock_gets(char *data, int len, int fd);

# endif
