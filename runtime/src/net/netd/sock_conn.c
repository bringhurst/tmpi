/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <string.h>
# include "tmpi_debug.h"
# include "sock_conn.h"

# define TCP_NODELAY 1

/**
 * Get a list of hostent structures for a given host name
 */
static struct hostent *sock_gethostbyname(char *host)
{
	char *xhost;
	char lhost[1024];

	if (!host)
		return NULL;

	if (strcmp(host, ".") == 0) {
		strncpy(lhost, sock_hostname(), 1023);
		lhost[1023]='\0';
		xhost=lhost;
	}
	else {
		xhost=host;
	}
		
	return gethostbyname(host);
}

/**
 * Connect to a specific server at specific port
 */
int sock_conn(char *host, int port)
{
	struct hostent *hp;
	struct sockaddr_in serv;
	const int max_retry=10; /* is 10 retrial enough? */
	int connected=0;
	int fd=-1;
	int n_addr;
	int i;
	struct linger ll;

	if (port<=0)
		return -1;

	if (port==SOCK_PORT_ANY) {
		tmpi_error(DBG_INFO, "server port cannot be SOCK_PORT_ANY");
		return -1;
	}

	hp=sock_gethostbyname(host);

	if (!hp) {
		tmpi_error(DBG_INFO, "cannot get server address (%s)", host);
		return -1;
	}

	n_addr=0;
	while (hp->h_addr_list[n_addr])
		n_addr++;

	memset((char *)&serv, 0, sizeof(serv));
	serv.sin_family=hp->h_addrtype;
	serv.sin_port=htons(port);

	for (i=0; i<n_addr; i++) {
		int retry=max_retry;

		memcpy((char *)&(serv.sin_addr), (char *)(hp->h_addr_list[i]), hp->h_length);

		while (retry--) {
			int flag=-1;

			fd=socket(AF_INET, SOCK_STREAM, 0);

			if (fd<0) {
				sleep(1);
				continue;
			}

			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));

			/**
			ll.l_onoff=1;
			ll.l_linger=0;

			setsockopt(fd, SOL_SOCKET, SO_LINGER, &ll, sizeof(ll));
			**/

			if (connect(fd, (struct sockaddr *)&serv, sizeof(serv))<0) {
				close(fd);
				fd=-1;
				sleep(1);
				continue;
			}
			else {
				connected=1;
				break;
			}
		}

		if (connected)
			break;
	}

	if (!connected) {
		if (fd>=0) {
			close(fd);
			fd=-1;
		}
		tmpi_error(DBG_INFO, "cannot connect to server (%s:%d)", host, port);

		return -1;
	}

	return fd;
}

/**
 * connect to the server with the client binding to a
 * specific address.
 */
int sock_conn2(char *host, int port, char *bind_host)
{
	struct hostent *hp;
	struct hostent *bhp;
	struct sockaddr_in serv;
	const int max_retry=10; /* is 5 retrial enough? */
	int connected=0;
	int fd=-1;
	int n_addr;
	int i;
	struct sockaddr_in sin;
	struct linger ll;
	int dobind=0;

	if (port<=0)
		return -1;

	if (port==SOCK_PORT_ANY) {
		tmpi_error(DBG_INFO, "server port cannot be SOCK_PORT_ANY");
		return -1;
	}

	if ( (!bind_host) || (strcmp(bind_host, ".")==0) ) {
		dobind=0;
	}
	else {
		dobind=1;
	}

	if (dobind) {
		bhp=sock_gethostbyname(bind_host);

		if (!bhp) {
			tmpi_error(DBG_INFO, "cannot get the address of local interface (%s)", bind_host);
			return -1;
		}

		sin.sin_family = AF_INET;
		memcpy((char *)&(sin.sin_addr), (char *)(bhp->h_addr_list[0]), bhp->h_length);
		sin.sin_port = htons(0);
	}

	hp=sock_gethostbyname(host);
	if (!hp) {
		tmpi_error(DBG_INFO, "cannot get server address (%s)", host);
		return -1;
	}

	n_addr=0;
	while (hp->h_addr_list[n_addr])
		n_addr++;

	memset((char *)&serv, 0, sizeof(serv));
	serv.sin_family=hp->h_addrtype;
	serv.sin_port=htons(port);

	for (i=0; i<n_addr; i++) {
		int retry=max_retry;

		memcpy((char *)&(serv.sin_addr), (char *)(hp->h_addr_list[i]), hp->h_length);

		while (retry--) {
			int flag=-1;

			fd=socket(AF_INET, SOCK_STREAM, 0);

			if (fd<0) {
				sleep(1);
				continue;
			}

			if (dobind) {
				if (bind(fd, (struct sockaddr *)&sin, sizeof(sin))<0) {
					if (fd>=0) {
						close(fd);
						fd=-1;
					}

					tmpi_error(DBG_INFO, "cannot bind to the specific interface (%s)", bind_host);

					return -1;
				}
			}

			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));

			/**
			ll.l_onoff=1;
			ll.l_linger=0;

			setsockopt(fd, SOL_SOCKET, SO_LINGER, &ll, sizeof(ll));
			**/

			if (connect(fd, (struct sockaddr *)&serv, sizeof(serv))<0) {
				close(fd);
				fd=-1;
				sleep(1);
				continue;
			}
			else {
				connected=1;
				break;
			}
		}

		if (connected)
			break;
	}

	if (!connected) {
		if (fd>=0) {
			close(fd);
			fd=-1;
		}

		tmpi_error(DBG_INFO, "cannot connect to server (%s:%d)", host, port);

		return -1;
	}

	return fd;
}

/**
 * Set the server to listen to a specific port.
 */
int sock_listen(int port, int backlog)
{
	int fd=-1;
	struct sockaddr_in sin;
	int flag=-1;
	struct linger ll;

	fd=socket(AF_INET, SOCK_STREAM, 0);

	if (fd==-1) {
		tmpi_error(DBG_INFO, "cannot open listening socket (%d)", port);
		return -1;
	}

	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));

	/**
	ll.l_onoff=1;
	ll.l_linger=0;

	setsockopt(fd, SOL_SOCKET, SO_LINGER, &ll, sizeof(ll));
	**/

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	if (port == SOCK_PORT_ANY)
		sin.sin_port = htons(0);
	else
		sin.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin))<0) {
		tmpi_error(DBG_INFO, "failed to bind to local interface (local_host:%d)", port);
		close(fd);
		return -1;
	}

	if (listen(fd, backlog)<0) {
		tmpi_error(DBG_INFO, "listen failed (local_host:%d)", port);
		close(fd);
		return -1;
	}

	return fd;
}

/**
 * Set the server to listen to a specific port with the address
 * bound to a specific host.
 */
int sock_listen2(int port, int backlog, char *bind_host)
{
	int fd=-1;
	struct sockaddr_in sin;
	int flag=-1;
	struct hostent *hp;
	struct linger ll;

	if (!bind_host)
		return -1;

	fd=socket(AF_INET, SOCK_STREAM, 0);

	if (fd==-1) {
		tmpi_error(DBG_INFO, "cannot open listening socket (%d)", port);
		return -1;
	}

	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));

	/**
	ll.l_onoff=1;
	ll.l_linger=0;

	setsockopt(fd, SOL_SOCKET, SO_LINGER, &ll, sizeof(ll));
	**/

	sin.sin_family = AF_INET;

	if ((!bind_host) || (strcmp(bind_host, ".")==0)) {
		sin.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		hp=sock_gethostbyname(bind_host);

		if (!hp) {
			tmpi_error(DBG_INFO, "cannot get the address for local interface (%s)", bind_host);
			close(fd);
			return -1;
		}

		memcpy((char *)&(sin.sin_addr), (char *)(hp->h_addr_list[0]), hp->h_length);
	}

	if (port == SOCK_PORT_ANY)
		sin.sin_port = htons(0);
	else
		sin.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin))<0) {
		tmpi_error(DBG_INFO, "failed to bind to local interface (%s:%d)", bind_host, port);
		close(fd);
		return -1;
	}
	if (listen(fd, backlog)<0) {
		tmpi_error(DBG_INFO, "listen failed (%s:%d)", bind_host, port);
		close(fd);
		return -1;
	}

	return fd;
}

int sock_accept(int fd)
{
	struct sockaddr_in from;
	int sinlen=sizeof(from);
	int newfd;

	if (fd<0)
		return -1;

	newfd=accept(fd, (struct sockaddr *)&from, &sinlen);

	if (newfd<0) {
		tmpi_error(DBG_INFO, "accept failed.");
		return -1;
	}

	return newfd;
}

char *sock_hostname()
{
	static char hostname[1024];

	if (gethostname(hostname, 1024)<0)
		return NULL;

	return hostname;
}

int sock_myport(int fd)
{
	struct sockaddr_in sin;
	int sinlen=sizeof(sin);

	if (getsockname(fd, (struct sockaddr *)&sin, &sinlen)<0)
		return -1;

	return ntohs(sin.sin_port);
}

int sock_peerport(int fd)
{
	struct sockaddr_in sin;
	int sinlen=sizeof(sin);

	if (getpeername(fd, (struct sockaddr *)&sin, &sinlen)<0)
		return -1;

	return ntohs(sin.sin_port);
}

char *sock_myip(int fd)
{
	struct sockaddr_in sin;
	int sinlen=sizeof(sin);

	if (getsockname(fd, (struct sockaddr *)&sin, &sinlen)<0)
		return NULL;

	return inet_ntoa(sin.sin_addr);
}

char *sock_peerip(int fd)
{
	struct sockaddr_in sin;
	int sinlen=sizeof(sin);

	if (getpeername(fd, (struct sockaddr *)&sin, &sinlen)<0)
		return NULL;

	return inet_ntoa(sin.sin_addr);
}

char *sock_peerhost(int fd)
{
	struct sockaddr_in sin;
	int sinlen=sizeof(sin);
	struct hostent *hent;

	if (getpeername(fd, (struct sockaddr *)&sin, &sinlen)<0)
		return NULL;

	hent=gethostbyaddr((char *)&(sin.sin_addr), sizeof(sin.sin_addr), AF_INET);
	if (!hent)
		return inet_ntoa(sin.sin_addr);

	return hent->h_name;
}
