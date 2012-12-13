# ifndef _MACHINE_H_
# define _MACHINE_H_
/** can be number or SIZE_UNKNOWN **/
# define SIZE_UNKNOWN -1
# define SIZEOF_INT 4
# define SIZEOF_LONG 8
# define SIZEOF_PTR 4

# define CACHE_LINE 64

int cpu_current();
int cpu_total();

# endif
