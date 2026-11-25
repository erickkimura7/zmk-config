/*
 * Reset-reason history, stored in flash, for debugging reboots that happen
 * while there is no USB log (e.g. running on battery).
 *
 * At every boot it records:
 *   - POWER->RESETREAS (why the chip reset; 0 = power-on / brown-out)
 *   - whether VBUS (USB power) was present at boot
 *   - the Zephyr fatal error code, if the previous boot crashed
 * and keeps the last 16 entries in the settings (NVS) partition.
 * The USB-logging build prints the whole history every 10 s.
 */

#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <soc.h>

LOG_MODULE_REGISTER(reset_reason, CONFIG_LOG_DEFAULT_LEVEL);

#define HIST_LEN 16
#define FATAL_MARK 0xA0 /* upper nibble marks GPREGRET2 as written by us */

struct rr_entry {
    uint32_t boot;
    uint32_t resetreas;
    uint8_t vbus;
    uint8_t fatal; /* 0 = none, else FATAL_MARK | k_fatal_error_reason */
} __packed;

static struct {
    uint32_t count;
    struct rr_entry e[HIST_LEN];
} hist;

static uint32_t cur_resetreas;
static uint8_t cur_vbus;
static uint8_t cur_fatal;

/* Capture the hardware state before anything else can touch it. */
static int rr_capture(void) {
    cur_resetreas = NRF_POWER->RESETREAS;
    NRF_POWER->RESETREAS = cur_resetreas; /* write-1-to-clear */
    cur_vbus = (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) ? 1 : 0;
    uint32_t g2 = NRF_POWER->GPREGRET2;
    if ((g2 & 0xF0) == FATAL_MARK) {
        cur_fatal = (uint8_t)g2;
    }
    NRF_POWER->GPREGRET2 = 0;
    return 0;
}
SYS_INIT(rr_capture, PRE_KERNEL_1, 0);

/* On a fatal error, remember the reason (survives the soft reset) and reboot. */
void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf) {
    ARG_UNUSED(esf);
    NRF_POWER->GPREGRET2 = FATAL_MARK | (reason & 0x0F);
    LOG_PANIC();
    sys_reboot(SYS_REBOOT_COLD);
    CODE_UNREACHABLE;
}

static int rr_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "h") == 0 && len == sizeof(hist)) {
        return read_cb(cb_arg, &hist, sizeof(hist)) < 0 ? -EIO : 0;
    }
    return -ENOENT;
}
SETTINGS_STATIC_HANDLER_DEFINE(rrdbg, "rrdbg", NULL, rr_settings_set, NULL, NULL);

static const char *reason_str(uint32_t r, char *buf, size_t len) {
    if (r == 0) {
        return "POWER-ON/BROWNOUT";
    }
    buf[0] = '\0';
    if (r & POWER_RESETREAS_RESETPIN_Msk) strncat(buf, "RESETPIN ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_DOG_Msk) strncat(buf, "WATCHDOG ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_SREQ_Msk) strncat(buf, "SOFT(SREQ) ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_LOCKUP_Msk) strncat(buf, "LOCKUP ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_OFF_Msk) strncat(buf, "WAKE-FROM-OFF ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_VBUS_Msk) strncat(buf, "WAKE-VBUS ", len - strlen(buf) - 1);
    if (r & POWER_RESETREAS_DIF_Msk) strncat(buf, "DEBUG ", len - strlen(buf) - 1);
    return buf;
}

static void rr_report(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_work, rr_report);

static void rr_report(struct k_work *work) {
    char buf[96];
    uint32_t n = MIN(hist.count, HIST_LEN);
    LOG_WRN("=== reset history: %u boots total, last %u (oldest first) ===", hist.count, n);
    for (uint32_t i = 0; i < n; i++) {
        const struct rr_entry *e = &hist.e[(hist.count - n + i) % HIST_LEN];
        LOG_WRN("boot #%u: %s(0x%08x) power=%s%s%u", e->boot,
                reason_str(e->resetreas, buf, sizeof(buf)), e->resetreas,
                e->vbus ? "USB" : "BATTERY", e->fatal ? " CRASH-before-this-boot reason=" : "",
                e->fatal ? (e->fatal & 0x0F) : 0);
    }
    k_work_schedule(&report_work, K_SECONDS(10));
}

/* Record this boot once ZMK has had time to bring up settings. */
static void rr_record(struct k_work *work) {
    settings_load_subtree("rrdbg");
    hist.count++;
    hist.e[(hist.count - 1) % HIST_LEN] = (struct rr_entry){
        .boot = hist.count, .resetreas = cur_resetreas, .vbus = cur_vbus, .fatal = cur_fatal};
    int err = settings_save_one("rrdbg/h", &hist, sizeof(hist));
    if (err) {
        LOG_ERR("reset history save failed: %d", err);
    }
    k_work_schedule(&report_work, K_SECONDS(1));
}
static K_WORK_DELAYABLE_DEFINE(record_work, rr_record);

static int rr_start(void) {
    k_work_schedule(&record_work, K_SECONDS(3));
    return 0;
}
SYS_INIT(rr_start, APPLICATION, 99);
