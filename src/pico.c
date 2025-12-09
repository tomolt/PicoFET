#include <pico.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>
#include <pico/status_led.h>
#include <hardware/gpio.h>

#include "fetcore.h"
#include "jtaglib.h"

struct pico_dev {
	struct jtdev jtdev; // must be the first member
	int pin_tck;
	int pin_tms;
	int pin_tdi;
	int pin_tdo;
	int pin_rst;
	int pin_tst;
	int pin_tclk;
};

void pico_dev_tck(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_tck, out);
}

void pico_dev_tms(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_tms, out);
}

void pico_dev_tdi(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_tdi, out);
}

void pico_dev_rst(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_rst, out);
}

void pico_dev_tst(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_tst, out);
}

int pico_dev_tdo_get(struct jtdev *p) {
	return gpio_get(((struct pico_dev *)p)->pin_tdo);
}

void pico_dev_tclk(struct jtdev *p, int out) {
	gpio_put(((struct pico_dev *)p)->pin_tclk, out);
}

int pico_dev_tclk_get(struct jtdev *p) {
	return gpio_get(((struct pico_dev *)p)->pin_tclk);
}

void pico_dev_tclk_strobe(struct jtdev *p, unsigned int count) {
	while (count) {
		gpio_put(((struct pico_dev *)p)->pin_tclk, 0);
		gpio_put(((struct pico_dev *)p)->pin_tclk, 1);
		count--;
	}
}

void pico_dev_led_green(struct jtdev *p, int out) {
	status_led_set_state(out);
}

void pico_dev_led_red(struct jtdev *p, int out) {
	(void)p;
	(void)out;
}

int pico_dev_open(struct jtdev *p, const char *device) {
	(void)p;
	(void)device;
	return 0;
}

void pico_dev_close    (struct jtdev *p) { (void)p; }
void pico_dev_power_on (struct jtdev *p) { (void)p; }
void pico_dev_power_off(struct jtdev *p) { (void)p; }
void pico_dev_connect  (struct jtdev *p) { (void)p; }
void pico_dev_release  (struct jtdev *p) { (void)p; }

const struct jtdev_func pico_dev_func = {
	.jtdev_open      = pico_dev_open,
	.jtdev_close     = pico_dev_close,
	.jtdev_power_on  = pico_dev_power_on,
	.jtdev_power_off = pico_dev_power_off,
	.jtdev_connect   = pico_dev_connect,
	.jtdev_release   = pico_dev_release,

	.jtdev_tck = pico_dev_tck,
	.jtdev_tms = pico_dev_tms,
	.jtdev_tdi = pico_dev_tdi,
	.jtdev_rst = pico_dev_rst,
	.jtdev_tst = pico_dev_tst,
	.jtdev_tdo_get = pico_dev_tdo_get,

	.jtdev_tclk        = pico_dev_tclk,
	.jtdev_tclk_get    = pico_dev_tclk_get,
	.jtdev_tclk_strobe = pico_dev_tclk_strobe,

	.jtdev_led_green = pico_dev_led_green,
	.jtdev_led_red   = pico_dev_led_red,

	.jtdev_ir_shift     = jtag_default_ir_shift,
	.jtdev_dr_shift_8   = jtag_default_dr_shift_8,
	.jtdev_dr_shift_16  = jtag_default_dr_shift_16,
	.jtdev_tms_sequence = jtag_default_tms_sequence,
	.jtdev_init_dap     = jtag_default_init_dap,
};

int main() {
	stdio_usb_init();
	status_led_init();

	struct pico_dev dev = {
		.jtdev = {
			.f = &pico_dev_func,
			.status = STATUS_OK,
		},
		.pin_tck = 0,
		.pin_tms = 0,
		.pin_tdi = 0,
		.pin_tdo = 0,
		.pin_rst = 0,
		.pin_tst = 0,
		.pin_tclk = 0,
	};

	for (;;) {
		command_loop(&dev.jtdev);
	}
	return 0;
}
