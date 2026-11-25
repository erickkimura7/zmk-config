/*
 * Force the nRF52840 DC/DC regulators off at boot.
 *
 * Zephyr's nRF52 SoC init only *enables* DCDC when the corresponding Kconfig
 * option is set; it never disables it. The bootloader runs first and may leave
 * DCDCEN0 (REG0, high-voltage stage) and/or DCDCEN (REG1) enabled. On clone
 * boards whose inductors differ from the nice!nano, that can make the board
 * unstable on battery while it works on USB.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_power.h>

LOG_MODULE_REGISTER(force_dcdc_off, CONFIG_LOG_DEFAULT_LEVEL);

static bool reg0_was_on;
static bool reg1_was_on;

static int force_dcdc_off(void) {
#if defined(CONFIG_ZMK_FORCE_DCDC_HV_OFF)
    reg0_was_on = nrf_power_dcdcen_vddh_get(NRF_POWER);
    nrf_power_dcdcen_vddh_set(NRF_POWER, false);
#endif
#if defined(CONFIG_ZMK_FORCE_DCDC_OFF)
    reg1_was_on = nrf_power_dcdcen_get(NRF_POWER);
    nrf_power_dcdcen_set(NRF_POWER, false);
#endif
    return 0;
}

/* Runs as early as possible, before the radio and clocks are started. */
SYS_INIT(force_dcdc_off, PRE_KERNEL_1, 0);

/* Repeat the report every 10 s: the boot-time message is usually gone before
 * a USB log reader can open the serial port. */
static void report_dcdc_state(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_work, report_dcdc_state);

static void report_dcdc_state(struct k_work *work) {
    LOG_WRN("DCDC at boot (left by bootloader): REG0/HV=%s REG1=%s | now: REG0/HV=%s REG1=%s",
            reg0_was_on ? "ON" : "off", reg1_was_on ? "ON" : "off",
            nrf_power_dcdcen_vddh_get(NRF_POWER) ? "ON" : "off",
            nrf_power_dcdcen_get(NRF_POWER) ? "ON" : "off");
    k_work_schedule(&report_work, K_SECONDS(10));
}

static int start_report(void) {
    k_work_schedule(&report_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(start_report, APPLICATION, 99);
