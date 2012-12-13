# ifndef _BARRIER_H_
# define _BARRIER_H_

# include "thread.h"

/* barrier control object */
typedef struct {
  	int maxcnt;           /* maximum number of runners    */
  	struct _sb {
    		thr_cnd_t wait_cv;     /* cv for waiters at barrier    */
    		thr_mtx_t wait_lk;    /* mutex for waiters at barrier */
    		int runners;        /* number of running threads    */
  	} sb[2];
  	struct _sb *sbp;      /* current sub-barrier          */
} tmpi_Bar;

int bar_init(tmpi_Bar *bp, int count);
int bar_wait(tmpi_Bar *bp);
int bar_destroy(tmpi_Bar *bp);

# endif
