
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

void pru_free_context(struct pru_context* pctx, enum pru_icss_index pru_num) {
        if (!pctx) return;

        if (pctx->pclk) iounmap(pctx->pclk);
        if (pctx->pcfg) iounmap(pctx->pcfg);
        if (pctx->piram) iounmap(pctx->piram);
        pctx->pclk = NULL;
        pctx->pcfg = NULL;
        pctx->piram = NULL;
}
