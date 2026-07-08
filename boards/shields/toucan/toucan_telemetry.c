/*
 * TOUCAN keyboard — telemetry module
 *
 * Streams live layer/keypress events over a second USB CDC-ACM serial port
 * so the PC overlay (TOUCAN Keymap Editor) can highlight the active layer
 * and pressed keys in real time.
 *
 * Wire protocol — ASCII lines, LF-terminated, sent to toucan_telemetry_uart:
 *   TELE_READY\n  emitted on the first key event (confirms module is running)
 *   L:<n>\n       highest active layer is now n
 *   D:<n>\n       ZMK matrix position n pressed
 *   U:<n>\n       ZMK matrix position n released
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

static const struct device *tele_dev;
static bool tele_ready = false;
static bool tele_greeting_sent = false;

static void tele_write(const char *buf, int len)
{
    if (!tele_ready || len <= 0) {
        return;
    }
    /* uart_poll_out on Zephyr CDC-ACM returns immediately when no host is
     * consuming data — safe to call unconditionally without DTR check. */
    for (int i = 0; i < len; i++) {
        uart_poll_out(tele_dev, buf[i]);
    }
}

static int on_layer_state_changed(const zmk_event_t *eh)
{
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "L:%d\n", zmk_keymap_highest_layer_active());
    tele_write(buf, n);
    return ZMK_EV_EVENT_BUBBLE;
}

static int on_position_state_changed(const zmk_event_t *eh)
{
    /* Send a one-time greeting on the first key event so the host can confirm
     * the telemetry module is running even without a layer change. */
    if (!tele_greeting_sent) {
        tele_greeting_sent = true;
        tele_write("TELE_READY\n", 11);
    }

    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%c:%d\n", ev->state ? 'D' : 'U', ev->position);
    tele_write(buf, n);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(toucan_tele_layer, on_layer_state_changed);
ZMK_SUBSCRIPTION(toucan_tele_layer, zmk_layer_state_changed);

ZMK_LISTENER(toucan_tele_pos, on_position_state_changed);
ZMK_SUBSCRIPTION(toucan_tele_pos, zmk_position_state_changed);

static int toucan_telemetry_init(void)
{
    tele_dev = DEVICE_DT_GET(DT_NODELABEL(toucan_telemetry_uart));
    if (!device_is_ready(tele_dev)) {
        return -ENODEV;
    }
    tele_ready = true;
    return 0;
}

SYS_INIT(toucan_telemetry_init, APPLICATION, 99);
