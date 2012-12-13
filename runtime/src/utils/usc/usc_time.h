# ifndef _TSC_TIME_H_
# define _TSC_TIME_H_

typedef long long usc_time_t;

typedef struct {
	usc_time_t elaps;
	usc_time_t lstart;
	int started;
} usc_tmr_t;

/** this needs to be called first **/
void usc_init();

double usc_difftime(usc_time_t, usc_time_t);

/** get the time in us related to usc_init **/
double usc_wtime(usc_time_t);

/**
 * get a reference point, use difftime to get the
 * meaningful time
 **/
usc_time_t usc_gettime();

/**
 * return the cpu speed in MHZ
 */
double usc_mhzrate();

void usc_tmr_reset(usc_tmr_t *timer);
void usc_tmr_start(usc_tmr_t *timer);
void usc_tmr_stop(usc_tmr_t *timer);
double usc_tmr_read(usc_tmr_t *timer);

# endif
