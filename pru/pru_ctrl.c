
#include "pru_ctrl.h"
#include <linux/errno.h>
#include <linux/ioport.h>
#include <rtdm/driver.h>

int pru_claim_memory_regions(void) {
        struct resource* res = NULL;

        rtdm_printk(KERN_INFO "Requesting memory regions\n");
        res = request_mem_region(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4,
                                 "PRU-ICSS1 CLK CTRL REG");
        if (!res) goto do_exit;
        res = request_mem_region(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE,
                                 "PRU-ICSS1 CTRL REG");
        if (!res) goto do_free_clkctrl;
        res = request_mem_region(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE,
                                 "PRU-ICSS1 PRU0 IRAM");
        if (!res) goto do_free_cfg;
        res = request_mem_region(PRUSS1_PRU0_DRAM_ADDR, PRUSS_PRU_DRAM_SIZE,
                                 "PRU-ICSS1 PRU0 DRAM");
        if (!res) goto do_free_iram;

        rtdm_printk(KERN_INFO "Memory regions requested successfully\n");

        return 0;

do_free_iram:
        release_mem_region(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE);
do_free_cfg:
        release_mem_region(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
do_free_clkctrl:
        release_mem_region(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
do_exit:
        rtdm_printk(KERN_INFO "Memory regions released\n");
        return -ENOMEM;
}

void pru_release_memory_regions(void) {
        rtdm_printk(KERN_INFO "Releasing memory regions\n");
        release_mem_region(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
        release_mem_region(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
        release_mem_region(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE);
        release_mem_region(PRUSS1_PRU0_DRAM_ADDR, PRUSS_PRU_DRAM_SIZE);
}

int pru_init_context(struct pru_context* pctx, enum pru_icss_index pru_num) {
        int err = -EINVAL;
        if (!pctx) goto exit_failure;
        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;
        pctx->pdram = NULL;
        pctx->ram_target = PRU_ACCESS_IRAM;

        if (pru_num == PRU_ICSS1) {
                pctx->pclk = ioremap(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
                pctx->pcfg = ioremap(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
                pctx->piram =
                    ioremap(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE);
                pctx->pdram =
                    ioremap(PRUSS1_PRU0_DRAM_ADDR, PRUSS_PRU_DRAM_SIZE);
                if (pctx->pclk && pctx->pcfg && pctx->piram && pctx->pdram)
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
        if (pctx->pdram) iounmap(pctx->pdram);

        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;
        pctx->pdram = NULL;

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
        if (pctx->pdram) iounmap(pctx->pdram);
        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;
        pctx->pdram = NULL;
}

void pru_set_device_state_async(struct pru_context* pctx,
                                enum pru_device_state state) {
        uint32_t tmp = ioread32(pctx->pclk);
        tmp &= ~0x3;
        if (state == PRU_STATE_ENABLED) tmp |= 2;

        iowrite32(tmp, pctx->pclk);
}

nanosecs_rel_t pru_wait_for_device_state(struct pru_context* pctx,
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
        } while (!is_expected_state && delta_ns <= timeout_ns);

        return timeout_ns - delta_ns;
}

enum pru_device_state pru_get_device_state(struct pru_context* pctx) {
        uint32_t regval = ioread32(pctx->pclk);
        if ((regval & (0x3 << 16)) == 0x3 << 16) return PRU_STATE_DISABLED;
        return PRU_STATE_ENABLED;
}