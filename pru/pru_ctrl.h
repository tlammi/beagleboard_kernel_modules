#ifndef _PRU_CTRL_H
#define _PRU_CTRL_H

#include <linux/io.h>

// Address definitions
#define PRUSS1_SLAVE_PORT_ADDR 0x4b200000
#define PRUSS1_CFG_REG_ADDR 0x4b226000
#define PRUSS2_CFG_REG_ADDR 0x4b2a6000

#define PRUSS_CFG_REG_SIZE 68

#define CM_L4PER2_PRUSS1_CLKCTRL_ADDR 0x4a009718

#define PRUSS1_PRU0_IRAM_ADDR (PRUSS1_SLAVE_PORT_ADDR + 0x34000)
#define PRUSS_PRU_IRAM_SIZE (12 * 1024)

struct pru_context {
        void* pclk;
        void* pcfg;
        void* piram;
};

enum pru_icss_index { PRU_ICSS1 = 0, PRU_ICSS2 };

int pru_init_context(struct pru_context* pctx, enum pru_icss_index pru_num);

void pru_free_context(struct pru_context* pctx, enum pru_icss_index pru_num);

int pru_load_program(struct pru_context* pctx, void* psrc);

// int pru_set_state(struct pru_context* pmap, enum pru_state_t new_state);

// enum pru_state_t pru_get_state(struct pru_context* pmap);

#endif  // _PRU_CTRL_H