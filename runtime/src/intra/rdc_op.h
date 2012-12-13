# ifndef _RDC_OP_H_
# define _RDC_OP_H_

void rdc_int_max(void *result, void *source, int count);
void rdc_int_min(void *result, void *source, int count);
void rdc_int_sum(void *result, void *source, int count);
void rdc_flt_max(void *result, void *source, int count);
void rdc_flt_min(void *result, void *source, int count);
void rdc_flt_sum(void *result, void *source, int count);
void rdc_dbl_max(void *result, void *source, int count);
void rdc_dbl_min(void *result, void *source, int count);
void rdc_dbl_sum(void *result, void *source, int count);

# endif
