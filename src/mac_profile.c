/*
 * Mac profile: turn the "Mac" keymap layer on automatically while a given
 * Bluetooth profile is selected, and off on any other profile.
 *
 * ZMK does not persist layer state, so a plain &tog would be lost every time
 * the keyboard wakes from deep sleep. Tying the layer to the BLE profile (which
 * IS persisted) keeps Mac mode across sleeps and reboots.
 *
 * Runs only on the central (left) half, where the keymap is processed.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_REGISTER(mac_profile, CONFIG_LOG_DEFAULT_LEVEL);

#define MAC_LAYER CONFIG_ZMK_MAC_PROFILE_LAYER
#define MAC_PROFILE CONFIG_ZMK_MAC_PROFILE_INDEX

static void apply_profile(int index) {
    if (index == MAC_PROFILE) {
        zmk_keymap_layer_activate(MAC_LAYER);
    } else {
        zmk_keymap_layer_deactivate(MAC_LAYER);
    }
    LOG_INF("BLE profile %d -> Mac layer %s", index, index == MAC_PROFILE ? "ON" : "off");
}

static int mac_profile_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    if (ev) {
        apply_profile(ev->index);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(mac_profile, mac_profile_listener);
ZMK_SUBSCRIPTION(mac_profile, zmk_ble_active_profile_changed);

/* At boot ZMK restores the selected profile from settings but does not raise
 * the profile-changed event, so check it once settings have been loaded. */
static void boot_check(struct k_work *work) { apply_profile(zmk_ble_active_profile_index()); }
static K_WORK_DELAYABLE_DEFINE(boot_work, boot_check);

static int mac_profile_init(void) {
    k_work_schedule(&boot_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(mac_profile_init, APPLICATION, 99);
