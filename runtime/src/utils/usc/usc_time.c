#include <stdio.h>
#include <string.h>
#include <time.h>
#include "usc_time.h"

static double MHZ_RATE=(double)400; /* in case people forgot to initialize the timer */
static long long USC_START=0;

static int mhz_rate_init() {
	double res=400.0, mhz;
	char line[256], *s, search_str[] = "cpu MHz";
	FILE *f;

	/* open proc/cpuinfo */
	if((f = fopen("/proc/cpuinfo", "r")) == NULL)
		return -1;

	/* ignore all lines until we reach MHz information */
	while(fgets(line, 256, f) != NULL){
		if(strstr(line, search_str) != NULL){
			/* ignore all characters in line up to : */
			for(s = line; *s && (*s != ':'); ++s);
			/* get MHz number */
			if(*s && (sscanf(s+1, "%lf", &mhz) == 1)){
				res = mhz;
				break;
			}
		}
	}

	fclose(f);

	if (MHZ_RATE==(double)-1)
		MHZ_RATE=res;

	return 0;
}

long long usc_gettime(void)
{
  unsigned int tmp[2];

  __asm__ ("rdtsc"
	   : "=a" (tmp[1]), "=d" (tmp[0])
	   : "c" (0x10) );
  
  return ( ((long long)tmp[0] << 32 | tmp[1]) ) - USC_START ;
}

double usc_difftime(long long time2, long long time1)
{
	if (MHZ_RATE == (double)-1) 
		mhz_rate_init();
	return (time2-time1)>0 ? (time2-time1)/MHZ_RATE : (time1-time2)/MHZ_RATE ;
}

double usc_wtime(long long t)
{
	return t/MHZ_RATE;
}

void usc_init(){
	mhz_rate_init();

	if (USC_START==(long long)0)
		USC_START=usc_gettime();
}

double usc_mhzrate()
{
	return MHZ_RATE;
}

void usc_tmr_reset(usc_tmr_t *timer)
{
	if (timer) {
		timer->elaps=0;
		timer->started=0;
	}
}

void usc_tmr_start(usc_tmr_t *timer)
{
	if (timer && (!timer->started)) {
		timer->started=1;
		timer->lstart=usc_gettime();
	}
}

void usc_tmr_stop(usc_tmr_t *timer)
{
	if (timer && (timer->started)) {
		timer->started=0;
		timer->elaps+=(usc_gettime()-timer->lstart);
	}
}

double usc_tmr_read(usc_tmr_t *timer)
{
	if (timer && (!timer->started)) {
		return usc_wtime(timer->elaps);
	}
	else if (timer && (timer->started)) {
		return usc_wtime(timer->elaps+(usc_gettime()-timer->lstart));
	}

	return 0.0;
}

