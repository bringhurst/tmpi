/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/uio.h>
# include <unistd.h>
# include <poll.h>

# ifndef POLLRDNORM
# define POLLRDNORM 0
# endif

# ifndef POLLWRNORM
# define POLLWRNORM 0
# endif

# include <errno.h>
# include "tmpi_debug.h"
# include "sock_sr.h"

# define MAX_RETRY 100

/**
 * No need to control rate after using TCP_NODELAY
# define MAX_READ (10240000)
# define MAX_WRITE (40960000*2)
**/

# define MIN(a,b) (((a)>(b))?(b):(a))
/**
 * one can keep alive a connection by sending
 * packets with payload size 0 or advatising
 * a 0 sized receive window. Here, I am
 * only going to retry after a 0 sized packet
 * for MAX_RETRY times.
 */
int sock_write(int fd, char *data, int len)
{
	int n=0;
	int m;
	int retry=0;
	// int counter=0;

	while (n<len) {
		// m=write(fd, data+n, MIN((len-n),MAX_WRITE));
		m=write(fd, data+n, (len-n));
		if (m==-1) { /* is it because of user signal? */
			if (errno!=EINTR) {
				return -1;
			}

			retry++;
			if (retry>=MAX_RETRY)
				return -1;

			continue;
		}

		if (m==0) {
			retry++;
			if (retry>=MAX_RETRY)
				return -1;
		}
		else {
			n+=m;
			// counter++;
		}
	}

	// tmpi_error(DBG_INFO, "total write = %d, average write size = %d.", len, len/counter);

	return 0;
}

/* might change iov! */
int	sock_writev(int fd, struct iovec *iov, int count)
{
	int n=0;
	int m;
	int retry=0;
	int len=0, i=0;

	for (; i<count; i++)
		len+=iov[i].iov_len;

	while (n<len) {
		m=writev(fd, iov, count);
		if (m==-1) { /* is it because of user signal? */
			if (errno!=EINTR) {
				return -1;
			}
			retry++;
			if (retry>=MAX_RETRY)
				return -1;

			continue;
		}

		if (m==0) {
			retry++;
			if (retry>=MAX_RETRY)
				return -1;
		}
		else {
			n+=m;

			/* adjusting iov */
			for (i=0; i<count; i++) {
				int t;

				if (m>iov[i].iov_len) {
					t=iov[i].iov_len;
				}
				else {
					t=m;
				}
				
				iov[i].iov_len-=t;
				iov[i].iov_base+=t;
				m-=t;

				if (m==0)
					break;
			}

			if (m!=0) {
				/* weired */
				return -1;
			}

			/* adjust iov */
			for (i=0; i<count; i++)
				if (iov[i].iov_len>0)
					break;

			iov+=i;
			count-=i;
		}
	}

	return 0;
}

int	sock_read(int fd, char *data, int len)
{
	int n=0;
	int m;
	int retry=0;
	// int counter=0;

	while (n<len) {
		// m=read(fd, data+n, MIN((len-n), MAX_READ));
		// m=recv(fd, data+n, MIN((len-n), MAX_READ), MSG_WAITALL);
		m=recv(fd, data+n, (len-n), MSG_WAITALL);
		if (m==-1) { /* is it because of user signal? */
			if (errno!=EINTR) {
				return -1;
			}
			retry++;
			if (retry>=MAX_RETRY)
				return -1;

			continue;
		}

		if (m==0) {
			retry++;
			if (retry>=MAX_RETRY)
				return -1;
		}
		else {
			n+=m;
			// counter++;
		}
	}

	// tmpi_error(DBG_INFO, "total read = %d, average read size = %d.", len, len/counter);

	return 0;
}

int	sock_read_till_close(int fd, char *data, int len)
{
	int n=0;
	int m;
	int retry=0;

	while (n<len) {
		m=recv(fd, data+n, (len-n), 0);
		if (m==-1) { /* is it because of user signal? */
			if (errno!=EINTR) {
				return n;
			}
			retry++;
			if (retry>=MAX_RETRY)
				return n;

			continue;
		}

		if (m==0) {
			retry++;
			if (retry>=MAX_RETRY)
				return n;

			break;
		}
		else {
			n+=m;
		}
	}

	return n;
}

/* readmax return the length of the actual data received, up to len */
int sock_readmax(int fd, char *data, int len)
{
	return read(fd, data, len);
}

int sock_canread(int fd)
{
	struct pollfd fdst;
	int ret;

	fdst.fd=fd;
	fdst.events=POLLIN|POLLRDNORM;
	fdst.revents=0;

	ret=poll(&fdst, 1, 0);

	if (ret==-1)
		return -1;

	if (ret==0)
		return 0;

	if (fdst.revents & (POLLIN|POLLRDNORM))
		return 1;
	return -1;
}

int sock_canwrite(int fd)
{
	struct pollfd fdst;
	int ret;

	fdst.fd=fd;
	fdst.events=POLLOUT|POLLWRNORM;
	fdst.revents=0;

	ret=poll(&fdst, 1, 0);

	if (ret==-1)
		return -1;

	if (ret==0)
		return 0;

	if (fdst.revents & (POLLOUT|POLLWRNORM))
		return 1;
	return -1;
}

char *sock_gets(char *buf, int buf_size, int istream)
{
	int  idx;
	int  status;
	int retry=0;

	if (!buf)
		return NULL;

	buf[0]='\0';
	buf[buf_size-1]='\0';

	for (idx=0; idx<buf_size-1; idx++) {
		/* it is inefficient to read one byte at a time */
		/* will change it to a buffer management layer to improve it */
		status=read(istream,buf+idx,1);
		if (status<0) {
			if (errno!=EINTR) {
				*(buf+idx)='\0';
				break;
			}

			retry++;
			if (retry>=MAX_RETRY) {
				*(buf+idx)='\0';
				break;
			}
			else
				continue;
		}

		if (status==0) { /* connection closed */
			*(buf+idx)='\0';
			break;
		}

		if (*(buf+idx)=='\n') {
			*(buf+idx+1)='\0';
			break;
		}
	}

	if (buf[0]=='\0')
		return NULL;

	return buf;
}
