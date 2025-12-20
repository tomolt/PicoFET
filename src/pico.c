#include <pico.h>
#include <pico/status_led.h>
#include <hardware/watchdog.h>
#include <hardware/gpio.h>
#include <tusb.h>

#include "fetcore.h"
#include "jtaglib.h"

#define MAX_COMMAND_LENGTH 256

unsigned char fet_buffer[FET_BUFFER_CAPACITY];

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

void pico_dev_led_green(__unused struct jtdev *p, int out) {
	status_led_set_state(out);
}

void pico_dev_led_red(__unused struct jtdev *p, __unused int out) {
}

int pico_dev_open(__unused struct jtdev *p, __unused const char *device) {
	return 0;
}

void pico_dev_close    (__unused struct jtdev *p) {}
void pico_dev_power_on (__unused struct jtdev *p) {}
void pico_dev_power_off(__unused struct jtdev *p) {}
void pico_dev_connect  (__unused struct jtdev *p) {}
void pico_dev_release  (__unused struct jtdev *p) {}

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

size_t tusb_transport_read_nb(__unused struct transport *t, void *buf, size_t max) {
	watchdog_update();
	tud_task();
	if (tud_cdc_connected() && tud_cdc_available()) {
		return tud_cdc_read(buf, max);
	} else {
		return 0;
	}
}

void tusb_transport_write(__unused struct transport *t, const void *buf, size_t max) {
	while (max) {
		watchdog_update();

		unsigned avail = tud_cdc_write_available();
		if (!avail) {
			sleep_ms(10);
			continue;
		}

		unsigned step = max < avail ? max : avail;
		tud_cdc_write(buf, step);
		tud_task();
		tud_cdc_write_flush();

		buf  = (const char *)buf + step;
		max -= step;
	}
}

void tusb_transport_flush_out(__unused struct transport *t) {
	do {
		tud_task();
	} while (tud_cdc_write_flush());
}

struct transport_func tusb_transport_func = {
	.read_nb   = tusb_transport_read_nb,
	.write     = tusb_transport_write,
	.flush_out = tusb_transport_flush_out,
};

static char line[MAX_COMMAND_LENGTH];

int main() {
	status_led_init();

	tusb_init();
	while (!tud_cdc_connected()) {
		tud_task();
		sleep_ms(10);
	}
	sleep_ms(10);

	struct pico_dev dev = {
		.jtdev = {
			.f = &pico_dev_func,
			.status = STATUS_OK,
		},
		.pin_tck = 0,
		.pin_tms = 0,
		.pin_tdi = 0,
		.pin_tdo = 1,
		.pin_rst = 0,
		.pin_tst = 0,
		.pin_tclk = 0,
	};

	struct transport tran = {
		.f = &tusb_transport_func,
	};

	if (watchdog_caused_reboot()) {
		send_status(&tran, STATUS_PROGRAMMER_FROZE);
	}

	// Even though the SDK documentation says that the watchdog timeout
	// is in milliseconds, it really seems to be in microseconds.
	watchdog_enable(5 * 1000 * 1000, true);

	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_init(1);
	gpio_set_dir(1, GPIO_IN);
	
	size_t buffered = 0;
	for (;;) {
		char *lf, c;

		tud_task();

		buffered += tran.f->read_nb(&tran, line + buffered, sizeof line - buffered);

		while ((lf = memchr(line, '\n', buffered))) {
			*lf = '\0';

			watchdog_update();
			process_command(&dev.jtdev, &tran, line);
			tran.f->flush_out(&tran);
			buffered -= lf + 1 - line;
			memmove(line, lf + 1, buffered);
		}

		if (buffered == sizeof line) {
			// discard any further input until we have reached a new line
			do {
				tran.f->read_nb(&tran, &c, 1);
			} while (c != '\n');
			buffered = 0;
			send_status(&tran, STATUS_COMMAND_TOO_LONG);
		}
	}

	return 0;
}
