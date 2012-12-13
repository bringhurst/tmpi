/*
 * Copyright (c) 2000 
 * University of California, Santa Barbara
 * All rights reserved.
 *
 * Author: Hong Tang
 */ 

# ifndef _SOCK_CONN_H_
# define _SOCK_CONN_H_

# define SOCK_PORT_ANY (0)

/*---------------------- client API ----------------------*/
/* return fd on success or -1 on error */
int sock_conn(char *host, int port);
int sock_conn2(char *host, int port, char *bind_host);

/*---------------------- server API ----------------------*/
/* return fd on success or -1 on error */
int sock_listen(int port, int backlog);
int sock_listen2(int port, int backlog, char *bind_host);

/* fd is returned by sock_listen */
int sock_accept(int fd);

/*---------------------- common API ----------------------*/
char *sock_hostname(void); /* my host name */
int sock_myport(int fd);
int sock_peerport(int fd);
char *sock_myip(int fd);
char *sock_peerip(int fd);
char *sock_peerhost(int fd);

# endif
