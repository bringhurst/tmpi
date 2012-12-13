# include "rdc_op.h"
# include "atomic.h"
# include "tmpi_debug.h"

/**
 * essentially it is the same as the global reduce ops, 
 * except that now the result is shared among multiple
 * nodes, so we have to use atomic operations to do it.
 */

void rdc_int_max(void *result, void *source, int count)
{
	int *res=(int *)result;
	int *src=(int *)source;
	register int i, tmp1, tmp2;

	for (i=0; i<count; i++) {
		do {
			tmp1=res[i];
			tmp2=src[i];

			if (tmp2<=tmp1)
				break;
		} while (!cas32(&(res[i]), tmp1, tmp2));
	}
}

void rdc_int_min(void *result, void *source, int count)
{
	int *res=(int *)result;
	int *src=(int *)source;
	register int i, tmp1, tmp2;

	for (i=0; i<count; i++) {
		do {
			tmp1=res[i];
			tmp2=src[i];

			if (tmp2>=tmp1)
				break;
		} while (!cas32(&(res[i]), tmp1, tmp2));
	}
}

void rdc_int_sum(void *result, void *source, int count)
{
	// int *res=(int *)result;
	int *res=(int *)result;
	int *src=(int *)source;
	register int i;

	for (i=0; i<count; i++) {
		aadd32(&(res[i]), src[i]);
	}
}

void rdc_flt_max(void *result, void *source, int count)
{
	float *res=(float *)result;
	float *src=(float *)source;
	union {
		float f;
		int i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(float)==4) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i];
	
				if (tmp2.f<=tmp1.f)
					break;
			} while (!cas32((int *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, float size not equal to 4!");
	}
}

void rdc_flt_min(void *result, void *source, int count)
{
	float *res=(float *)result;
	float *src=(float *)source;
	union {
		float f;
		int i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(float)==4) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i];
	
				if (tmp2.f>=tmp1.f)
					break;
			} while (!cas32((int *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, float size not equal to 4!");
	}
}

void rdc_flt_sum(void *result, void *source, int count)
{
	float *res=(float *)result;
	float *src=(float *)source;
	union {
		float f;
		int i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(float)==4) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i]+tmp1.f;
			} while (!cas32((int *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, float size not equal to 4!");
	}
}

void rdc_dbl_max(void *result, void *source, int count)
{
	double *res=(double *)result;
	double *src=(double *)source;
	union {
		double f;
		long long i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(double)==8) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i];
	
				if (tmp2.f<=tmp1.f)
					break;
			} while (!cas64((long long *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, double size not equal to 4!");
	}
}

void rdc_dbl_min(void *result, void *source, int count)
{
	double *res=(double *)result;
	double *src=(double *)source;
	union {
		double f;
		long long i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(double)==8) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i];
	
				if (tmp2.f>=tmp1.f)
					break;
			} while (!cas64((long long *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, double size not equal to 4!");
	}
}

void rdc_dbl_sum(void *result, void *source, int count)
{
	double *res=(double *)result;
	double *src=(double *)source;
	union {
		double f;
		long long i;
	} tmp1, tmp2;
	register int i;

	if (sizeof(double)==8) {
		for (i=0; i<count; i++) {
			do {
				tmp1.f=res[i];
				tmp2.f=src[i]+tmp1.f;
			} while (!cas64((long long *)&(res[i]), tmp1.i, tmp2.i));
		}
	}
	else {
		tmpi_error(DBG_INTERNAL, "Architecture assumption failed, double size not equal to 4!");
	}
}
