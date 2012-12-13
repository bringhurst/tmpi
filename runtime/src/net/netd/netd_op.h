# ifndef _NETD_OP_H_
# define _NETD_OP_H_

/* Standard operations on double */
void netd_dbl_sum_op(void *x, void *y, int nelem);
void netd_dbl_mult_op(void *x, void *y, int nelem);
void netd_dbl_max_op(void *x, void *y, int nelem);
void netd_dbl_min_op(void *x, void *y, int nelem);
void netd_dbl_absmax_op(void *x, void *y, int nelem);
void netd_dbl_absmin_op(void *x, void *y, int nelem);

/* Standard operations on floats */
void netd_flt_sum_op(void *x, void *y, int nelem);
void netd_flt_mult_op(void *x, void *y, int nelem);
void netd_flt_max_op(void *x, void *y, int nelem);
void netd_flt_min_op(void *x, void *y, int nelem);
void netd_flt_absmax_op(void *x, void *y, int nelem);
void netd_flt_absmin_op(void *x, void *y, int nelem);


/* Standard operations on integers */
void netd_int_sum_op(void *x, void *y, int nelem);
void netd_int_mult_op(void *x, void *y, int nelem);
void netd_int_max_op(void *x, void *y, int nelem);
void netd_int_min_op(void *x, void *y, int nelem);
void netd_int_absmax_op(void *x, void *y, int nelem);
void netd_int_absmin_op(void *x, void *y, int nelem);

/* abs vector operations */
void netd_int_abs_op(void *x, int nelem);
void netd_flt_abs_op(void *x, int nelem);
void netd_dbl_abs_op(void *x, int nelem);

# endif
