#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#include "nec_decoder.h"

static struct gpio_callback ir_receiver_cb_data;
static const struct gpio_dt_spec ir_receiver = GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), ir_receiver_gpios, {0});

static uint32_t ir_prev_cycles = 0;
static uint32_t ir_decoded_value = 0;
static bool ir_new_decoded_value = false;

void ir_receiver_pulse(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	uint32_t cycles = k_cycle_get_32();
	uint32_t pulse_duration_us = (uint32_t)((cycles - ir_prev_cycles) / (float)sys_clock_hw_cycles_per_sec() * 1000000);
	ir_prev_cycles = cycles;

	if (nec_decoder_add_pulse(pulse_duration_us, &ir_decoded_value)) {
		ir_new_decoded_value = true;
	}
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&ir_receiver)) {
		printk("Error: IR device %s is not ready\n", ir_receiver.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&ir_receiver, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, ir_receiver.port->name, ir_receiver.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&ir_receiver, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, ir_receiver.port->name, ir_receiver.pin);
		return 0;
	}

	gpio_init_callback(&ir_receiver_cb_data, ir_receiver_pulse, BIT(ir_receiver.pin));
	gpio_add_callback(ir_receiver.port, &ir_receiver_cb_data);
	printk("Set up IR decoder at %s pin %d\n", ir_receiver.port->name, ir_receiver.pin);

	while (1) {
		if (ir_new_decoded_value) {
			ir_new_decoded_value = false;
			printk("Received code: 0x%08" PRIx32 "\n", ir_decoded_value);
		}

		k_msleep(500);
	}

	return 0;
}
