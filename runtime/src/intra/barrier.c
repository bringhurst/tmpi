# include "thread.h"
# include "barrier.h"

int bar_init(tmpi_Bar *bp, int count) {
  int n, i;

  if (count<1) return -1;

  bp->maxcnt = count;
  bp->sbp = &bp->sb[0];

  for (i=0; i<2; ++i) {
    struct _sb *sbp=&(bp->sb[i]);
    sbp->runners = count;

    if ( (n=thr_mtx_init(&sbp->wait_lk)) )
      return n;

    if ( (n=thr_cnd_init(&sbp->wait_cv)) )
      return n;
  }
  return 0;
}

int bar_wait(tmpi_Bar *bp) {
  register struct _sb *sbp = bp->sbp;

  thr_mtx_lock( &sbp->wait_lk );

  if ( sbp->runners == 1 ) {      /* last thread to reach barrier */
    if ( bp->maxcnt != 1 ) {
      /* reset runner count and switch sub-barriers   */
      sbp->runners=bp->maxcnt;
      bp->sbp=(bp->sbp == &bp->sb[0] )? &bp->sb[1] : &bp->sb[0];

      /* wake up the waiters */
      thr_cnd_bcast( &sbp->wait_cv );
    }
  } 
  else {
    sbp->runners--;         /* one less runner */
    
    while ( sbp->runners != bp->maxcnt ) /* important! */
      thr_cnd_wait( &sbp->wait_cv, &sbp->wait_lk );
  }

  thr_mtx_unlock( &sbp->wait_lk );

  return 0;
}

/* bar_destroy - destroy a barrier variable. */
int bar_destroy(tmpi_Bar *bp) {
  int n, i;

  for (i=0; i<2; ++i ) {
    if ( ( n = thr_cnd_destroy( &bp->sb[i].wait_cv ) ) )
      return n;

    if ( ( n = thr_mtx_destroy( &bp->sb[i].wait_lk ) ) )
      return n;
  }

  return 0 ;
}
