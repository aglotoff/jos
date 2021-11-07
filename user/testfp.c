#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	double e = 2.718281828;

	cprintf("%.0f\n", e);
	cprintf("%.0f.\n", e);
	cprintf("%.1f\n", e);
	cprintf("%.2f\n", e);
	cprintf("%.3f\n", e);
	cprintf("%f\n", e);
	cprintf("%.7f\n", e);

	cprintf("%5.1f\n", e);
	cprintf("%05.1f\n", e);
}
