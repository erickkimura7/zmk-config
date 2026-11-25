/*
 * Keep the 32 MHz crystal (HFXO) running permanently.
 *
 * These clone boards have no 32.768 kHz crystal, so the LF clock is
 * synthesized from the HF clock (CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH).
 * That is only accurate while the HFXO runs. On USB the USB peripheral keeps
 * the HFXO on all the time, so BLE is stable; on battery the HF clock is
 * released between radio events, the synthesized LF clock drifts, and the
 * split link drops with supervision timeouts (reason 0x08).
 *
 * Holding an HF clock request forever fixes the LF clock at the cost of
 * roughly 0.25 mA.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

LOG_MODULE_REGISTER(keep_hfxo_on, CONFIG_LOG_DEFAULT_LEVEL);

static struct onoff_client hfxo_cli;

static int keep_hfxo_on(void) {
    struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

    sys_notify_init_spinwait(&hfxo_cli.notify);
    int err = onoff_request(mgr, &hfxo_cli);
    if (err < 0) {
        LOG_ERR("HFXO request failed: %d", err);
    }
    return 0;
}

SYS_INIT(keep_hfxo_on, APPLICATION, 0);
