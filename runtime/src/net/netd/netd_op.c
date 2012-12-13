# include "netd_op.h"

# define ABSVAL(x) ( ((x) > 0) ? (x) : (-(x)) )
# define MAXVAL(a, b) ( ((a) > (b)) ? (a) : (b) )
# define MINVAL(a, b) ( ((a) < (b)) ? (a) : (b) )

void netd_dbl_sum_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
	*a++ += *b++;
}

void netd_dbl_mult_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
	*a++ *= *b++;
}

void netd_dbl_max_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
    {
	*a = MAXVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_dbl_min_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
    {
	*a = MINVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_dbl_absmax_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
    {
	*a = MAXVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}

void netd_dbl_absmin_op(void *x, void *y, int nelem)
{
    double *a = (double *) x;
    double *b = (double *) y;

    while (nelem--)
    {
	*a = MINVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}

/* Standard operations on floats */

void netd_flt_sum_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
	*a++ += *b++;
}

void netd_flt_mult_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
	*a++ *= *b++;
}

void netd_flt_max_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
    {
	*a = MAXVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_flt_min_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
    {
	*a = MINVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_flt_absmax_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
    {
	*a = MAXVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}

void netd_flt_absmin_op(void *x, void *y, int nelem)
{
    float *a = (float *) x;
    float *b = (float *) y;

    while (nelem--)
    {
	*a = MINVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}


/* Standard operations on integers */

void netd_int_sum_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
	*a++ += *b++;
}

void netd_int_mult_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
	*a++ *= *b++;
}

void netd_int_max_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
    {
	*a = MAXVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_int_min_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
    {
	*a = MINVAL(*a, *b);
	a++;
	b++;
    }
}

void netd_int_absmax_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
    {
	*a = MAXVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}

void netd_int_absmin_op(void *x, void *y, int nelem)
{
    int *a = (int *) x;
    int *b = (int *) y;

    while (nelem--)
    {
	*a = MINVAL(ABSVAL(*a), ABSVAL(*b));
	a++;
	b++;
    }
}

void netd_int_abs_op(void *x, int nelem)
{
	int *a=(int *)x;

	while (nelem--)
	{
		*a = ABSVAL(*a);
		a++;
	}
}

void netd_flt_abs_op(void *x, int nelem)
{
	float *a=(float *)x;

	while (nelem--)
	{
		*a = ABSVAL(*a);
		a++;
	}
}

void netd_dbl_abs_op(void *x, int nelem)
{
	double *a=(double *)x;

	while (nelem--)
	{
		*a = ABSVAL(*a);
		a++;
	}
}
