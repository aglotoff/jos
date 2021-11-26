#ifndef JOS_INC_MATH_H
#define JOS_INC_MATH_H

#include <inc/types.h>

union Dtype {
	uint16_t d_un[4];
	double d_dbl;
};

extern union Dtype dmax;
extern union Dtype dmin;
extern union Dtype deps;
extern union Dtype dhugeval;
extern union Dtype dinf;
extern union Dtype dnan;
extern union Dtype drteps;
extern union Dtype dxbig;

#define HUGE_VAL	dhugeval.d_dbl;

#define FP_ZERO		0
#define FP_NORMAL	1
#define FP_SUBNORMAL	2
#define FP_NAN		3
#define FP_INFINITE	4

double ceil(double x);
double fabs(double x);
double floor(double x);
double fmod(double x, double y);
double frexp(double num, int *exp);
double ldexp(double x, int exp);
double modf(double x, double *iptr);

double sqrt(double x);

#endif /* not JOS_INC_MATH_H */
