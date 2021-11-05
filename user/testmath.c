// Test math functions

#include <inc/lib.h>
#include <inc/math.h>

static int
approx(double x, double y)
{
	return fabs((y != 0) ? (x - y) / y : x) < deps.d_dbl;
}

void
umain(int argc, char **argv)
{
	double x;
	int exp;

	assert(ceil(-5.1) == -5.0);
	assert(ceil(-5.0) == -5.0);
	assert(ceil(-4.9) == -4.0);
	assert(ceil(0.0) == 0.0);
	assert(ceil(4.9) == 5.0);
	assert(ceil(5.0) == 5.0);
	assert(ceil(5.1) == 6.0);

	assert(fabs(-5.0) == 5.0);
	assert(fabs(0.0) == 0.0);
	assert(fabs(5.0) == 5.0);

	assert(floor(-5.1) == -6.0);
	assert(floor(-5.0) == -5.0);
	assert(floor(-4.9) == -5.0);
	assert(floor(0.0) == 0.0);
	assert(floor(4.9) == 4.0);
	assert(floor(5.0) == 5.0);
	assert(floor(5.1) == 5.0);

	assert(fmod(-7.0, 3.0) == -1.0);
	assert(fmod(-3.0, 3.0) == 0.0);
	assert(fmod(-2.0, 3.0) == -2.0);
	assert(fmod(0.0, 3.0) == 0.0);
	assert(fmod(2.0, 3.0) == 2.0);
	assert(fmod(3.0, 3.0) == 0.0);
	assert(fmod(7.0, 3.0) == 1.0);

	assert(approx(frexp(-3.0, &exp), -0.75) && exp == 2);
	assert(approx(frexp(-0.5, &exp), -0.5) && exp == 0);
	assert(frexp(0.0, &exp) == 0.0 && exp == 0);
	assert(approx(frexp(0.33, &exp), 0.66) && exp == -1);
	assert(approx(frexp(0.66, &exp), 0.66) && exp == 0);
	assert(approx(frexp(96.0, &exp), 0.75) && exp == 7);

	assert(ldexp(-3.0, 4) == -48.0);
	assert(ldexp(-0.5, 0) == -0.5);
	assert(ldexp(0.0, 36) == 0.0);
	assert(approx(ldexp(0.66, -1), 0.33));
	assert(ldexp(96, -3) == 12.0);

	assert(approx(modf(-11.7, &x), -11.7 + 11.0) && x == -11.0);
	assert(modf(-0.5, &x) == -0.5 && x == 0.0);
	assert(modf(0.0, &x) == -0.0 && x == 0.0);
	assert(modf(0.6, &x) == 0.6 && x == 0.0);
	assert(modf(12.0, &x) == 0.0 && x == 12.0);

	cprintf("SUCCESS testing <math.h>, part 1\n");
}
