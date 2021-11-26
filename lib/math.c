// Basic math functions based on the reference implementation from the P.J. Plauger's
// "The Standard C Library".

#include <inc/math.h>

// The value of a double number is:
//
// (-1)^S * (1.TTT...) * 2^(EEE...-DBIAS)
//
// Double representation:
//
// [SEEEEEEEEEEETTTT]  [TTTT....TTTT]  [TTTT....TTTT]  [TTTT....TTTT]
//     x.d_un[D0]        x.d_un[D1]      x.d_un[D2]      x.d_un[D3]

// Parameters for 64-bit double values for the Intel 80x87 coprocessor
#define D0		3		// subscript of the most significant word
#define D1		(D0 - 1)
#define D2		(D1 - 1)
#define D3		(D2 - 1)
#define DOFF		4		// the number of fraction bits in D0
#define NBITS		(48 + DOFF)	// total number of fraction bits
#define DBIAS		1023		// bias added to the exponent

#define DSIGN		(1 << 15)		// sign bit
#define DFRAC		((1 << DOFF) - 1)	// fraction part in D0
#define DEXP		(0x7fff & ~DFRAC)	// exponent

// Maximum exponent value
#define DMAXE		((1 << (15 - DOFF)) - 1)

// NaN representation
#define DNAN		(DSIGN | (DMAXE << DOFF) | (1 << (DOFF - 1)))

// Initialize a double value
#define DINIT(u0, ux)	{ (ux), (ux), (ux), (u0) }

// Static values
union Dtype dmax	= { DINIT((DMAXE << DOFF) - 1, ~0) };
union Dtype dmin	= { DINIT(1 << DOFF, 0) };
union Dtype deps	= { DINIT((DBIAS - NBITS) << DOFF, 0) };
union Dtype dhugeval 	= { DINIT(DMAXE << DOFF, 0) };
union Dtype dinf 	= { DINIT(DMAXE << DOFF, 0) };
union Dtype dnan 	= { DINIT(DNAN, 0) };
union Dtype drteps 	= { DINIT((DBIAS - 1 + NBITS / 2) << DOFF, 0) };
union Dtype dxbig 	= { DINIT((DBIAS - 1 + NBITS / 2) << DOFF, 0) };


// --------------------------------------------------------------
// Floating-point manipulation functions
// --------------------------------------------------------------

//
// Classify a floating-point value as one of the following categories:
//   - zero
//   - normal
//   - subnormal
//   - infinite
//   - NaN
//
static int
dclassify(double *x)
{
	uint16_t *w;
	int e, frac;
	
	w = (uint16_t *) &x;
	e = (w[D0] & DEXP) >> DOFF;
	frac = (w[D0] & DFRAC) || w[D1] || w[D2] || w[D3];

	if (e == DMAXE)
		return frac ? FP_NAN : FP_INFINITE;
	if (e == 0)
		return frac ? FP_SUBNORMAL : FP_ZERO;
	return FP_NORMAL;
}

//
// Classify a floating-point value and drop all fraction bits less that the threshold
// value 2^texp.
//
static int
dtrunc(double *x, int texp)
{
	static uint16_t mask[] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000f, 0x001f, 0x003f, 0x007f,
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff, 0x1fff, 0x3fff, 0x7fff,
	};
	static int sub[] = { D3, D2, D1, D0 };

	uint16_t *w;
	int e, frac, dropbits, dropwords;
	
	w = (uint16_t *) x;
	e = (w[D0] & DEXP) >> DOFF;
	frac = (w[D0] & DFRAC) || w[D1] || w[D2] || w[D3];

	if (e == DMAXE)
		return frac ? FP_NAN : FP_INFINITE;
	if (e == 0 && !frac)
		return FP_ZERO;

	// Determine the number of fraction bits to drop.
	dropbits = NBITS - (e - DBIAS) - texp;
	
	if (dropbits <= 0)		// Nothing to drop.
		return FP_ZERO;

	if (dropbits >= NBITS) {	// Clear all fraction bits.
		w[D0] = w[D1] = w[D2] = w[D3] = 0;
		return e ? FP_NORMAL : FP_SUBNORMAL;
	}

	dropwords = dropbits >> 4;
	dropbits &= 0xF;

	// Clear fraction bits in the highest word.
	frac = w[sub[dropwords]] & mask[dropbits];
	w[sub[dropwords]] &= ~mask[dropbits];

	// Clear whole 16-bit words (falls through!).
	switch (dropwords) {
	case 3:
		frac |= w[D1], w[D1] = 0;
	case 2:
		frac |= w[D2], w[D2] = 0;
	case 1:
		frac |= w[D3], w[D3] = 0;
	}

	if (!frac)
		return FP_ZERO;
	return e ? FP_NORMAL : FP_SUBNORMAL;
}

//
// Normalize the fraction part of a gradual underflow and return the adjusted exponent
// value.
//
static int
dnorm(uint16_t *w)
{
	int e, sign;

	if (!(w[D0] & DFRAC) && (w[D1] == 0) && (w[D2] == 0) && (w[D3] == 0))
		return 0;

	// Remember the sign bit.
	sign = w[D0] & DSIGN;
	w[D0] &= DFRAC;

	// To improve performance, try to shift left by 16 bits at a time.
	for (e = 0; w[D0] == 0; e -= 16) {
		// Shift left by 16 bits (may overshoot)
		w[D0] = w[D1];
		w[D1] = w[D2];
		w[D2] = w[D3];
		w[D3] = 0;
	}

	// If we have overshoot, back up by shifting right.
	for ( ; w[D0] >= (1 << (DOFF + 1)); e++) {
		w[D3] = (w[D3] >> 1) | (w[D2] << 15);
		w[D2] = (w[D2] >> 1) | (w[D1] << 15);
		w[D1] = (w[D1] >> 1) | (w[D0] << 15);
		w[D0] = (w[D0] >> 1);
	}

	// Otherwise, continue shifting left by 1 bit at a time.
	for ( ; w[D0] < (1 << (DOFF)); e--) {
		w[D0] = (w[D0] << 1) | (w[D1] >> 15);
		w[D1] = (w[D1] << 1) | (w[D2] >> 15);
		w[D2] = (w[D2] << 1) | (w[D3] >> 15);
		w[D3] = (w[D3] << 1);
	}

	// Clear the implicit fraction bit and restore the sign bit.
	w[D0] = (w[D0] & DFRAC) | sign;

	return e;
}

//
// Scale the double value by 2^exp.
//
int
dscale(double *x, int exp)
{
	uint16_t *w;
	int e, frac, sign;
	
	w = (uint16_t *) x;
	e = (w[D0] & DEXP) >> DOFF;
	frac = (w[D0] & DFRAC) || w[D1] || w[D2] || w[D3];
	
	if (e == DMAXE)				// NaN or Infinity, do nothing.
		return frac ? FP_NAN : FP_INFINITE;
	
	if (e == 0 && (e = dnorm(w)) == 0)	// Zero value, do nothing.
		return FP_ZERO;

	// Adjust the exponent.
	e += exp;

	if (e >= DMAXE) {
		// Overflow, return +/- infinity.
		*x = w[D0] & DSIGN ? -dinf.d_dbl : dinf.d_dbl;
		return FP_INFINITE;
	}

	if (e > 0) {
		// Normal value, simply update the exponent field.
		w[D0] = (w[D0] & ~DEXP) | ((uint16_t) e << DOFF);
		return FP_NORMAL;
	}

	// Remember the sign bit.
	sign = w[D0] & DSIGN;
	w[D0] = (1 << DOFF) | (w[D0] & DFRAC);

	if (e >= -(NBITS + 1)) {
		// First, shift right by 16 bits at a time.
		for ( ; e <= -16; e += 16) {
			w[D3] = w[D2];
			w[D2] = w[D1];
			w[D1] = w[D0];
			w[D0] = 0;
		}

		// Then, scale by 1 bit at a time.
		if ((e = -e) != 0) {
			w[D3] = (w[D3] >> e) | (w[D2] << (16 - e));
			w[D2] = (w[D2] >> e) | (w[D1] << (16 - e));
			w[D1] = (w[D1] >> e) | (w[D0] << (16 - e));
			w[D0] = (w[D0] >> e);
		}

		// No underflow - return a subnormal value.
		if (w[D0] || w[D1] || w[D2] || w[D3]) {
			w[D0] |= sign;
			return FP_SUBNORMAL;
		}
	}

	// Underflow - return zero.
	w[D0] = sign;
	w[D1] = w[D2] = w[D3] = 0;
	return FP_ZERO;
}

//
// Break the double value into 0.5 <= |f| < 1.0 and 2^exp.
//
static int
dunscale(double *x, int *exp)
{
	uint16_t *w;
	int e, frac, ndrop, himask, hisub;
	
	w = (uint16_t *) x;
	e = (w[D0] & DEXP) >> DOFF;
	frac = (w[D0] & DFRAC) || w[D1] || w[D2] || w[D3];

	if (e == DMAXE) {
		// NaN or Infinity
		*exp = 0;
		return frac ? FP_NAN : FP_INFINITE;
	}

	if (e != 0 || ((e = dnorm(w)) != 0)) {
		// Normal value, simply update the exponent field.
		w[D0] = (w[D0] & ~DEXP) | ((DBIAS - 1) << DOFF);
		*exp = e + 1 - DBIAS;
		return FP_NORMAL;
	}

	// Zero.
	*exp = 0;
	return FP_ZERO;
}

double
ceil(double x)
{
	int t;

	if (((t = dtrunc(&x, 0)) == FP_NORMAL || t == FP_SUBNORMAL) && (x > 0.0))
		return x + 1.0;
	return x;
}

double
fabs(double x)
{
	switch (dclassify(&x)) {
	case FP_INFINITE:
		return dinf.d_dbl;
	case FP_NAN:
		return x;
	case FP_ZERO:
		return 0.0;
	default:
		return (x < 0.0) ? -x : x;
	}
}

double
floor(double x)
{
	int t;

	if (((t = dtrunc(&x, 0)) == FP_NORMAL || t == FP_SUBNORMAL) && (x < 0.0))
		return x - 1.0;
	return x;
}

double
fmod(double x, double y)
{
	double t;
	int xtype, ytype, xexp, yexp, sign, n;

	xtype = dclassify(&x);
	ytype = dclassify(&y);

	if (xtype == FP_NAN || xtype == FP_ZERO || ytype == FP_INFINITE)
		return x;
	if (ytype == FP_NAN)
		return y;
	if ((xtype == FP_INFINITE) || (ytype == FP_ZERO))
		return dnan.d_dbl;
	
	if (x < 0.0) {
		x = -x;
		sign = 1;
	} else {
		sign = 0;
	}

	if (y < 0.0)
		y = -y;
	
	t = y;
	n = 0;
	dunscale(&t, &yexp);

	// Repeatedly subtract |y| from |x| until the remainder is smaller than |y|.
	while (n >= 0) {
		// First, compare the exponents of |x| and |y| to determine whether or
		// not further subtraction might be possible.
		t = x;
		if ((dunscale(&t, &xexp) == FP_ZERO) || ((n = xexp - yexp) < 0))
			break;

		// Try to subtract |y|*2^n.
		for ( ; n >= 0; n--) {
			t = y;
			dscale(&t, n);
			if (t <= x) {
				x -= t;
				break;
			}
		}
	}

	return sign ? -x : x;
}

double
frexp(double num, int *exp)
{
	int binexp;

	switch (dunscale(&num, &binexp)) {
	case FP_NAN:
	case FP_INFINITE:
		*exp = 0;
		return num;
	case FP_ZERO:
		*exp = 0;
		return 0.0;
	default:
		*exp = binexp;
		return num;
	}
}

double
ldexp(double x, int exp)
{
	int t;

	if ((t = dclassify(&x)) == FP_NORMAL || t == FP_SUBNORMAL)
		dscale(&x, exp);
	return x;
}

double
modf(double x, double *iptr)
{
	*iptr = x;

	switch (dtrunc(iptr, 0)) {
	case FP_NAN:
		return x;
	case FP_INFINITE:
	case FP_ZERO:
		return 0.0;
	default:
		return x - *iptr;
	}
}

// sqrt(0.5)
#define SQRTHALF	0.70710678118654752440

double
sqrt(double x)
{
	int i, e;
	double y;

	if (x < 0)
		return dnan.d_dbl;

	// 1. Reduce x to f and e such that x = f * 2^e.
	switch (dunscale(&x, &e)) {
	case FP_NAN:
		return dnan.d_dbl;
	case FP_INFINITE:
		return dinf.d_dbl;
	case FP_ZERO:
		return 0.0;
	}

	// 2. Compute sqrt(f) using the Newton's Method:
	//   y(i) = (y(i-1) + f/y(i-1)) / 2,  i = 1,2,...,j

	// y(0) (Hart et al, Computer Approximations, 1968).
	y = 0.41731 + 0.59016 * x;

	// We can save one multiply when computing the value of y(2).
	y += x / y;
	y = 0.25 * y + x / y;

	// For 64-bit double valus, 3 iterations are sufficient.
	y = 0.5 * (y + x / y);

	// 3. Reconstruct the value of sqrt(x):
	//   sqrt(x) = sqrt(f) * 2^(e/2)      , if e is even
	// or
	//   (sqrt(f) / sqrt(2)) * 2^((e+1)/2), if e is odd.

	if (e & 1) {
		// Multiplication is usually faster than division, so we multiply by
		// sqrt(0.5) instead of dividing by sqrt(2).
		y *= SQRTHALF;
		e++;
	}

	dscale(&y, e/2);

	return y;
}
