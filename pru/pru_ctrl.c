
#include "pru_ctrl.h"
#include <linux/errno.h>
#include <linux/ioport.h>
#include <rtdm/driver.h>

int pru_init_context(struct pru_context* pctx, enum pru_icss_index pru_num) {
        int err = -EINVAL;
        if (!pctx) goto exit_failure;
        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;

        if (pru_num == PRU_ICSS1) {
                pctx->pclk = ioremap(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
                pctx->pcfg = ioremap(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
                pctx->piram =
                    ioremap(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE);
                if (pctx->pclk && pctx->pcfg && pctx->piram)
                        goto exit_success;
                else
                        err = -EIO;

        } else if (pru_num == PRU_ICSS2) {
                printk(KERN_ERR "PRU-ICSS2 is not yet supported\n");
        } else {
                printk(KERN_ERR "Unknown PRU index %i\n", pru_num);
        }

        if (pctx->pclk) iounmap(pctx->pclk);
        if (pctx->pcfg) iounmap(pctx->pcfg);
        if (pctx->piram) iounmap(pctx->piram);

        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;

exit_failure:
        rtdm_printk(KERN_ERR "Failed to initialize PRU context\n");
        return err;
exit_success:
        rtdm_printk(KERN_INFO "PRU context initialized\n");
        return 0;
}

void pru_free_context(struct pru_context* pctx) {
        if (!pctx) return;

        if (pctx->pclk) iounmap(pctx->pclk);
        if (pctx->pcfg) iounmap(pctx->pcfg);
        if (pctx->piram) iounmap(pctx->piram);
        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;
}

void pru_set_device_state_async(struct pru_context* pctx,
                                enum pru_device_state state) {
        uint32_t tmp = ioread32(pctx->pclk);
        tmp &= ~0x3;
        if (state == PRU_STATE_ENABLED) tmp |= 2;

        iowrite32(tmp, pctx->pclk);
}

int pru_wait_for_device_state(struct pru_context* pctx,
                              enum pru_device_state state,
                              nanosecs_rel_t timeout_ns) {
        int val = 0;
        bool is_expected_state = false;
        nanosecs_abs_t starttime = rtdm_clock_read_monotonic();
        nanosecs_rel_t delta_ns = 0;
        do {
                val = ioread32(pctx->pclk);
                bool is_enabled = !((val & (0x3 << 16)) && 0x3 << 16);
                if (state == PRU_STATE_ENABLED)
                        is_expected_state = is_enabled;
                else
                        is_expected_state = !is_enabled;

                delta_ns = rtdm_clock_read_monotonic() - starttime;
        } while (!is_expected_state && delta_ns < timeout_ns);

        if (is_expected_state)
                return 0;
        else
                return -EIO;
}

enum pru_device_state pru_get_device_state(struct pru_context* pctx) {
        uint32_t regval = ioread32(pctx->pclk);
        if (regval & (0x3) << 16 == 0x3 << 16) return PRU_STATE_DISABLED;
        return PRU_STATE_ENABLED;
}