/*****************************************************************************
 * @file   main.c
 * @brief  The start of application.
 *******************************************************************************
 Copyright 2022 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>

#include <openthread/thread.h>
#include <openthread/instance.h>

#ifdef CONFIG_BOARD_NRF52840DONGLE_NRF52840
#include <drivers/uart.h>
#include <usb/usb_device.h>
#endif

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <zephyr/dfu/mcuboot.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include "stat_mgmt/stat_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_SMP_UDP
#include "gl_smp_udp.h"
#endif
#ifdef CONFIG_LED_STRIP
#include "gl_led_strip.h"
#endif

#include "gl_cjson_utils.h"
#include "gl_qdec.h"
#include "gl_gpio.h"
#include "gl_led.h"
#include "gl_coap.h"
#include "gl_ot_api.h"
#include "gl_sensor.h"
#include "gl_button_logic.h"
#include "gl_types.h"
#include "gl_battery.h"

LOG_MODULE_REGISTER(main, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);


int joiner_state;


static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	if (joiner_state != DEVICE_CONNECTED) {
		LOG_WRN("device connected.");
		joiner_state = DEVICE_CONNECTED;
		led_toggle_stop();
	}

	dk_set_led_on(LED2);
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	if (joiner_state != DEVICE_DISCONNECTED) {
		LOG_WRN("device disconnected.");
		joiner_state = DEVICE_DISCONNECTED;
		led_toggle_stop();
		led_toggle_start(1000);
	}

	dk_set_led_off(LED2);
}

static void on_mtd_mode_toggle(uint32_t med)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	const struct device *cons = device_get_binding(CONSOLE_LABEL);

	if (med) {
		pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	} else {
		pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	}
#endif
	dk_set_led(LED2, med);
}

static struct k_work  qdec_work;
static struct k_timer qdec_idle_timer;
static volatile int32_t qdec_accum       = 0;
static volatile bool    qdec_send_pending = false;

#define QDEC_IDLE_MS 1000

/* ── Encoder LED mode state ─────────────────────────────────────────────
 * encoder_mode cycles on each button press of the rotary encoder:
 *   0 = LED1 hue       1 = LED1 brightness
 *   2 = LED2 hue       3 = LED2 brightness
 *
 * QDEC returns degrees; steps=60 in DTS means one full rotation = 360°.
 * Hue  : hue_deg += sv.val1  →  360° sweep = full rainbow in one turn.
 * Brightness: delta = sv.val1 * 254 / 360  →  360° = full 1-255 range.
 * ────────────────────────────────────────────────────────────────────── */
static uint8_t          encoder_mode      = 0;
static int16_t          led_hue_deg[2]    = {0, 0};  /* per-LED hue [0,359] */
static uint8_t          led_bri[2]        = {255, 255}; /* per-LED brightness */
static volatile int32_t led_pending_steps = 0;
static struct k_work    led_encoder_work;

/* Integer HSV→RGB.  S=1, V=1.  hue_deg in [0,360). */
static void hsv_to_rgb(int hue_deg, uint8_t *r, uint8_t *g, uint8_t *b)
{
	hue_deg = ((hue_deg % 360) + 360) % 360;
	int sector = hue_deg / 60;
	int frac   = (hue_deg % 60) * 255 / 60;
	int q      = 255 - frac;
	switch (sector) {
	case 0: *r = 255;  *g = frac; *b = 0;    break;
	case 1: *r = q;    *g = 255;  *b = 0;    break;
	case 2: *r = 0;    *g = 255;  *b = frac; break;
	case 3: *r = 0;    *g = q;    *b = 255;  break;
	case 4: *r = frac; *g = 0;    *b = 255;  break;
	default:*r = 255;  *g = 0;    *b = q;    break;
	}
}

static void qdec_idle_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_work_submit(&qdec_work);
}

static void qdec_trigger(struct k_work *item)
{
	ARG_UNUSED(item);

	/* Atomically capture and clear the accumulator. */
	unsigned int key = irq_lock();
	int32_t total    = qdec_accum;
	qdec_accum       = 0;
	qdec_send_pending = true;
	irq_unlock(key);

	if (total != 0) {
		double val = (double)total;
		send_trigger_event_request(QDEC_ROTATE_TRIGGER, "qdec_0", (void *)&val);
	}

	qdec_send_pending = false;
}

/* Called from gl_button_logic.c on each BTN4 press.
 * Cycles encoder_mode 0→1→2→3→4(off)→0 and controls the active LED.
 * Mode 4 turns both LEDs off.  Re-entering mode 0 resets both brightness
 * to 50% so the user gets a clean start after powering back on. */
void encoder_mode_cycle(void)
{
	encoder_mode = (encoder_mode + 1) % 5;
	static const char *const mode_names[] = {
		"LED1 color", "LED1 brightness",
		"LED2 color", "LED2 brightness",
		"all off",
	};
	printk("Encoder mode: %s\n", mode_names[encoder_mode]);
	if (encoder_mode == 4) {
		on_off_led_strip(LED_STRIP_NODE_1, LED_OFF);
		on_off_led_strip(LED_STRIP_NODE_2, LED_OFF);
	} else {
		if (encoder_mode == 0) {
			/* Returning from OFF — set both strips to 50% */
			led_bri[0] = 127;
			led_bri[1] = 127;
			set_led_strip_brightness(LED_STRIP_NODE_1, 127);
			set_led_strip_brightness(LED_STRIP_NODE_2, 127);
		}
		uint16_t node = (encoder_mode < 2) ? LED_STRIP_NODE_1 : LED_STRIP_NODE_2;
		on_off_led_strip(node, LED_ON);
	}
}

/* System-workqueue handler — applies accumulated encoder steps to the
 * active LED's hue or brightness and refreshes the hardware. */
static void led_encoder_apply(struct k_work *item)
{
	ARG_UNUSED(item);

	unsigned int key = irq_lock();
	int32_t steps    = led_pending_steps;
	led_pending_steps = 0;
	irq_unlock(key);

	if (steps == 0) {
		return;
	}

	if (encoder_mode == 4) {
		return; /* all-off mode — ignore rotation */
	}

	uint8_t  led_idx = (encoder_mode < 2) ? 0 : 1;
	uint16_t node    = (encoder_mode < 2) ? LED_STRIP_NODE_1 : LED_STRIP_NODE_2;
	bool     is_hue  = (encoder_mode == 0 || encoder_mode == 2);

	if (is_hue) {
		/* QDEC degrees map 1:1 to hue; one full rotation = full rainbow */
		int32_t new_hue = (int32_t)led_hue_deg[led_idx] + steps;
		new_hue = ((new_hue % 360) + 360) % 360;
		led_hue_deg[led_idx] = (int16_t)new_hue;

		uint8_t r, g, b;
		hsv_to_rgb(led_hue_deg[led_idx], &r, &g, &b);
		struct led_rgb color = { .r = r, .g = g, .b = b };
		update_led_strip_rgb(node, &color);
	} else {
		/* one full rotation (360°) = full brightness range 1-255 */
		int32_t delta   = steps * 254 / 360;
		int32_t new_bri = (int32_t)led_bri[led_idx] + delta;
		if (new_bri < 1)   { new_bri = 1; }
		if (new_bri > 255) { new_bri = 255; }
		led_bri[led_idx] = (uint8_t)new_bri;
		set_led_strip_brightness(node, led_bri[led_idx]);
	}

	on_off_led_strip(node, LED_ON);
}

void encoder_callback(const struct device *dev, const struct sensor_trigger *trigger)
{
	if (qdec_send_pending) {
		return;
	}

	struct sensor_value sv;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &sv);

	unsigned int key = irq_lock();
	led_pending_steps += sv.val1;
	qdec_accum        += sv.val1;
	/* Anti-windup: once at a brightness limit, stop accumulating steps
	 * in the same direction so the first step the other way takes effect. */
	if (encoder_mode == 1 || encoder_mode == 3) {
		uint8_t idx = (encoder_mode == 1) ? 0 : 1;
		if (led_bri[idx] >= 255 && led_pending_steps > 0) { led_pending_steps = 0; }
		if (led_bri[idx] <= 1   && led_pending_steps < 0) { led_pending_steps = 0; }
	}
	irq_unlock(key);

	k_work_submit(&led_encoder_work);
	/* Restart idle timer — fires QDEC_IDLE_MS after the last step. */
	k_timer_start(&qdec_idle_timer, K_MSEC(QDEC_IDLE_MS), K_NO_WAIT);
}

void print_version(void)
{
	printk("{\"Board\":\"%s\"}\n", CONFIG_BOARD);
	printk("{\"SW_Version\":\"%s\"}\n", CONFIG_SW_VERSION);
	printk("OpenThread Stack: %s\n", otGetVersionString());
	printk("OpenThread Mode: %s\n", ot_get_mode());
}

void simulation_click(uint32_t button)
{
	on_button_changed(0xFFFF, button);
	on_button_changed(0xFFFF, button);
}

void auto_join_commissioning_start()
{
	simulation_click(DK_BTN2_MSK);
}

void main(void)
{
	print_version();

	int ret;

#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
	stat_mgmt_register_group();
#endif

	ret = gl_gpio_init();
	if (ret) {
		LOG_ERR("Could not initialize gpios, err code: %d", ret);
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Could not initialize leds, err code: %d", ret);
	}
	light_off();

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
	}

	ret = battery_measure_enable(true);
	if (ret) {
		LOG_ERR("Could not initialize battery measurement, err code: %d", ret);
	}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/* Check if the image is run in the REVERT mode and eventually
	 * confirm it to prevent reverting on the next boot.
	 */
	if (mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT) {
		if (boot_write_img_confirmed()) {
			LOG_ERR("Confirming firmware image failed, it will be reverted on the next boot.");
		} else {
			LOG_INF("New device firmware image confirmed.");
		}
	}
#endif

	joiner_state = DEVICE_INITIAL;

	gl_sensor_init();

	coap_client_utils_init(on_ot_connect, on_ot_disconnect, on_mtd_mode_toggle);
	// auto_commissioning_timer_init();
	auto_join_commissioning_start();

#ifdef CONFIG_SENSOR_VALUE_AUTO_PRINT
	debug_sensor_data();
#endif
	gl_led_strip_init();

	k_work_init(&qdec_work, qdec_trigger);
	k_work_init(&led_encoder_work, led_encoder_apply);
	k_timer_init(&qdec_idle_timer, qdec_idle_expiry, NULL);
	gl_qdec_init(encoder_callback);

	return ;
}


