/*
 * TOUCAN keyboard — telemetry module
 *
 * Streams live layer/keypress events over a second USB CDC-ACM serial port
 * so the PC overlay (TOUCAN Keymap Editor) can highlight the active layer
 * and pressed keys in real time.
 *
 * Wire protocol — ASCII lines, LF-terminated, sent to toucan_telemetry_uart:
 *   L:<n>\n   highest active layer is now n  (0=BASE, 1=NAV, 2=SYM, 3=ADJ)
 *   D:<n>\n   ZMK matrix position n pressed
 *   U:<n>\n   ZMK matrix position n released
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

static const struct device *tele_dev;

static void tele_send(const char *buf, int len)
{
    if (len <= 0 || !tele_dev) {
        return;
    }
    /* Only write when a PC has the port open (host asserts DTR). */
    uint32_t dtr = 0;
    if (uart_line_ctrl_get(tele_dev, UART_LINE_CTRL_DTR, &dtr) < 0 || !dtr) {
        return;
    }
    for (int i = 0; i < len; i++) {
        uart_poll_out(tele_dev, buf[i]);
    }
}

static int on_layer_state_changed(const zmk_event_t *eh)
{
    char buf[8];
    int n = snprintf(buf, sizeof(buf), "L:%d\n", zmk_keymap_highest_layer_active());
    tele_send(buf, n);
    return ZMK_EV_EVENT_BUBBLE;
}

static int on_position_state_changed(const zmk_event_t *eh)
{
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    char buf[8];
    int n = snprintf(buf, sizeof(buf), "%c:%d\n", ev->state ? 'D' : 'U', ev->position);
    tele_send(buf, n);
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
    return 0;
}

SYS_INIT(toucan_telemetry_init, APPLICATION, 99);
