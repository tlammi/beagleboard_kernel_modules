
#include <linux/errno.h>
#include <linux/ioport.h>

#include "pru_ctrl.h"

static int _pru_map_mem_reqion(struct pru_reg_t* preg, uint32_t start,
                               uint32_t size) {
        preg->pres = request_mem_region(start, size, "");
        if (!preg->pres) {
                printk(KERN_ERR "request_mem_reqion() failed: %08x\n", start);
                return -EBUSY;
        }
        preg->pmap = ioremap(start, size);
        if (!preg->pmap) {
                printk(KERN_ERR "ioremap() failed: %08x\n", start);
                release_mem_region(start, size);
                return -EIO;
        }

        return 0;
}

static void _pru_unmap_mem_region(struct pru_reg_t* preg, uint32_t start,
                                  uint32_t size) {
        if (preg->pmap) {
                iounmap(preg->pmap);
                preg->pmap = NULL;
        }
        if (preg->pres) {
                release_mem_region(start, size);
                preg->pres = NULL;
        }
}

static int _pru_get_addresses(enum pru_icss_index pru_num, uint32_t* pclk_ctrl,
                              uint32_t* pprusscfg, uint32_t* piram) {
        if (pru_num == PRU_ICSS1) {
                *pclk_ctrl = CM_L4PER2_PRUSS1_CLKCTRL_ADDR;
                *pprusscfg = PRUSS1_CFG_REG_ADDR;
                *piram = PRUSS1_PRU0_IRAM_ADDR;
                return 0;
        } else if (pru_num == PRU_ICSS2) {
                printk(KERN_ERR "PRU-ICSS2 is not yet supported\n");
        } else {
                printk(KERN_ERR "Invalid PRU-ICSS index\n");
        }

        return -EINVAL;
}

int pru_init_context(struct pru_context* pctx, enum pru_icss_index pru_num) {
        if (!pctx) return -EINVAL;
        uint32_t clkctrl_addr, prusscfg_addr, pru0_iram_addr;

        int res = _pru_get_addresses(pru_num, &clkctrl_addr, &prusscfg_addr,
                                     &pru0_iram_addr);
        if (res) return res;

        res = _pru_map_mem_reqion(&pctx->clkctrl, clkctrl_addr, 4);
        if (res) goto exit_failure;

        res =
            _pru_map_mem_reqion(&pctx->cfg, prusscfg_addr, PRUSS_CFG_REG_SIZE);
        if (res) goto do_unmap_clk_ctrl;

        res = _pru_map_mem_reqion(&pctx->iram, pru0_iram_addr,
                                  PRUSS_PRU_IRAM_SIZE);
        if (res) goto do_unmap_pruss_cfg;
        return 0;

do_unmap_pruss_cfg:
        _pru_unmap_mem_region(&pctx->cfg, prusscfg_addr, PRUSS_CFG_REG_SIZE);
do_unmap_clk_ctrl:
        _pru_unmap_mem_region(&pctx->clkctrl, clkctrl_addr, 4);
exit_failure:
        return -EIO;
}

void pru_free_context(struct pru_context* pctx, enum pru_icss_index pru_num) {
        if (!pctx) return;

        uint32_t clkctrl_addr, prusscfg_addr, pru0_iram_addr;

        int res = _pru_get_addresses(pru_num, &clkctrl_addr, &prusscfg_addr,
                                     &pru0_iram_addr);
        if (res) return;
        _pru_unmap_mem_region(&pctx->iram, pru0_iram_addr, PRUSS_PRU_IRAM_SIZE);
        _pru_unmap_mem_region(&pctx->cfg, prusscfg_addr, PRUSS_CFG_REG_SIZE);
        _pru_unmap_mem_region(&pctx->clkctrl, clkctrl_addr, 4);
}
