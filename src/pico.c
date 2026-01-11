#include <pico.h>
#include <pico/status_led.h>
#include <hardware/watchdog.h>
#include <hardware/gpio.h>
#include <tusb.h>

#include "picofet_proto.h"
#include "jtaglib.h"
#include "jtdev.h"
#include "comm.h"
#include "cmd.h"

// JTAG device declaration & plumbing code

void pico_dev_tck(struct jtdev *p, int out) {
	gpio_put(p->pin_tck, out);
	sleep_us(2);
}

void pico_dev_tms(struct jtdev *p, int out) {
	gpio_put(p->pin_tms, out);
}

void pico_dev_tdi(struct jtdev *p, int out) {
	gpio_set_dir(p->pin_tdi, GPIO_OUT);
	gpio_put(p->pin_tdi, out);
}

void pico_dev_rst(struct jtdev *p, int out) {
	gpio_put(p->pin_rst, out);
}

void pico_dev_tst(struct jtdev *p, int out) {
	gpio_put(p->pin_tst, out);
}

int pico_dev_tdo_get(struct jtdev *p) {
	return gpio_get(p->pin_tdo);
}

void pico_dev_tclk(struct jtdev *p, int out) {
	gpio_set_dir(p->pin_tdi, GPIO_OUT);
	gpio_put(p->pin_tdi, out);
	sleep_us(2);
}

int pico_dev_tclk_get(struct jtdev *p) {
	gpio_set_dir(p->pin_tdi, GPIO_IN);
	return gpio_get(p->pin_tdi);
}

void pico_dev_tclk_strobe(struct jtdev *p, unsigned int count) {
	gpio_set_dir(p->pin_tdi, GPIO_OUT);
	while (count) {
		gpio_put(p->pin_tdi, 0);
		gpio_put(p->pin_tdi, 1);
		sleep_us(2);
		count--;
	}
}

void pico_dev_led_green(__unused struct jtdev *p, int out) {
	status_led_set_state(out);
}

void pico_dev_led_red(__unused struct jtdev *p, __unused int out) {}

void pico_dev_power_on (__unused struct jtdev *p) {}
void pico_dev_power_off(__unused struct jtdev *p) {}
void pico_dev_connect  (__unused struct jtdev *p) {}
void pico_dev_release  (__unused struct jtdev *p) {}

int pico_dev_open(struct jtdev *p, __unused const char *device) {
	extern const struct jtdev_func pico_dev_func;

	// Fill out the JTAG device structure

	p->f = &pico_dev_func;
	p->status = STATUS_OK;
	p->attached = false;
	p->pin_tck = 8;
	p->pin_tms = 9;
	p->pin_tdi = 13;
	p->pin_tdo = 12;
	p->pin_rst = 11;
	p->pin_tst = 10;

	// Initialize hardware pins accordingly

	status_led_init();

	gpio_init(p->pin_tck);
	gpio_set_dir(p->pin_tck, GPIO_OUT);
	
	gpio_init(p->pin_tms);
	gpio_set_dir(p->pin_tms, GPIO_OUT);
	
	gpio_init(p->pin_tdi);
	gpio_set_dir(p->pin_tdi, GPIO_OUT);
	
	gpio_init(p->pin_tdo);
	gpio_set_dir(p->pin_tdo, GPIO_IN);
	
	gpio_init(p->pin_rst);
	gpio_set_dir(p->pin_rst, GPIO_OUT);
	
	gpio_init(p->pin_tst);
	gpio_set_dir(p->pin_tst, GPIO_OUT);
	
	return 0;
}

void pico_dev_close(struct jtdev *p) {
	gpio_deinit(p->pin_tck);
	gpio_deinit(p->pin_tms);
	gpio_deinit(p->pin_tdi);
	gpio_deinit(p->pin_tdo);
	gpio_deinit(p->pin_rst);
	gpio_deinit(p->pin_tst);
}

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

// Communication with host via (Tiny)USB

void comm_tusb_open(struct comm *t) {
	extern struct comm_func comm_tusb_func;
	
	tusb_init();
	while (!tud_cdc_connected()) {
		tud_task();
		sleep_ms(10);
	}
	sleep_ms(10);

	t->f = &comm_tusb_func;

}

size_t comm_tusb_read_nb(__unused struct comm *t, void *buf, size_t max) {
	watchdog_update();
	tud_task();
	if (tud_cdc_connected() && tud_cdc_available()) {
		return tud_cdc_read(buf, max);
	} else {
		return 0;
	}
}

void comm_tusb_write(__unused struct comm *t, const void *buf, size_t max) {
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

void comm_tusb_flush_out(__unused struct comm *t) {
	watchdog_update();
	do {
		tud_task();
	} while (tud_cdc_write_flush());
}

struct comm_func comm_tusb_func = {
	.comm_open      = comm_tusb_open,
	.comm_read_nb   = comm_tusb_read_nb,
	.comm_write     = comm_tusb_write,
	.comm_flush_out = comm_tusb_flush_out,
};

// Initialization

int main() {
	// Even though the SDK documentation says that the watchdog timeout
	// is in milliseconds, it really seems to be in microseconds.
	watchdog_enable(5 * 1000 * 1000, true);

	struct comm comm;
	comm_tusb_func.comm_open(&comm);

	struct jtdev jtdev;
	pico_dev_func.jtdev_open(&jtdev, NULL);

	if (watchdog_caused_reboot()) {
		send_status(&comm, STATUS_PROGRAMMER_FROZE);
	}

	command_loop(&jtdev, &comm);

	// Not reached
	pico_dev_func.jtdev_close(&jtdev);

	return 0;
}
