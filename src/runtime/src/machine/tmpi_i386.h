#ifndef _I386_H_
#define _I386_H_

#ifdef __SMP__
#define LOCK_PREFIX "lock ; "
#else
#define LOCK_PREFIX ""
#endif

/* some hacks to defeat gcc over-optimizations */
struct __dummy { unsigned long a[100]; };
#define ADDR (*(volatile struct __dummy *)addr)

#endif /* _I386_MYTP_H */
