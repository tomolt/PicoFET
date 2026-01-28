#ifndef PICOFET_PHYS_H
#define PICOFET_PHYS_H

#include <stdbool.h>

struct phys_func;
struct phys {
	const struct phys_func *f;
};

struct phys_func {
	(*delay_sbw)();
	(*sbwtdio_set)();
	(*sbwtck_low)();
	(*sbwtck_high)();
	sbwtck_drive();

	void (*tms_set)();
	void (*tck_set)();
	void (*tdi_set)();
	bool (*tdo_get)();
	void (*rst_set)();
	void (*test_set)();
};

#define phys_tms_set(p, value) ((p)->f->tms_set(p, value))
#define phys_tck_set(p, value) ((p)->f->tck_set(p, value))
#define phys_tdi_set(p, value) ((p)->f->tdi_set(p, value))
#define phys_rst_set(p, value) ((p)->f->rst_set(p, value))
#define phys_test_set(p, value) ((p)->f->test_set(p, value))

#endif
