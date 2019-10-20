#ifndef _PRU_CTRL_H
#define _PRU_CTRL_H

#include <linux/io.h>

// Address definitions
#define PRUSS1_SLAVE_PORT_ADDR 0x4b200000
#define PRUSS1_CFG_ADDR 0x4b226000
#define PRUSS2_CFG_ADDR 0x4b2a6000

#define CM_L4PER2_PRUSS1_CLKCTRL_ADDR 0x4a009718

#define PRUSS1_PRU0_IRAM_ADDR (PRUSS1_SLAVE_PORT_ADDR + 0x34000)
#define PRUSS_PRU_IRAM_SIZE 12 * 1024;

struct pruss_cfg_t {
        uint32_t revid;
        uint32_t syscfg;
        uint32_t gpcfg0;
        uint32_t gpcfg1;
        uint32_t cgr;
        uint32_t isrp;
        uint32_t isp;
        uint32_t iesp;
        uint32_t iecp;
        uint32_t __reserved;
        uint32_t pmao;
        uint32_t mii_rt;
        uint32_t iepclk;
        uint32_t ssp;
        uint16_t __reserved2;
        uint32_t pin_mx;
} __attribute__((packed));

struct pru_context {
        struct resource* res_clck_ctrl;
        struct resource* res_pruss_cfg;
        struct resource* res_iram;
        uint32_t* clk_ctrl;
        struct pruss_cfg_t* pruss_cfg;
        void* iram;
};

enum pru_icss_index { PRU_ICSS1 = 0, PRU_ICSS2 };
enum pru_state_t { PRU_STATE_IDLE = 0, PRU_STATE_ACTIVE };

int pru_init_memory_mappings(struct pru_context* pmapping,
                             enum pru_icss_index pru_num);

int pru_free_memory_mappings(struct pru_context* pmapping,
                             enum pru_icss_index pru_num);

int pru_load_program(struct pru_context* pmap, void* psrc);

int pru_set_state(struct pru_context* pmap, enum pru_state_t new_state);
enum pru_state_t pru_get_state(struct pru_context* pmap);

/*int pru_set_module_mode(uint32_t new_mode) {
        if (new_mode != 0 && new_mode != 2) return -1;
        uint32_t val = CM_L4PER2_PRUSS1_CLKCTRL & ~0x3;
        CM_L4PER2_PRUSS1_CLKCTRL = val | new_mode;
        return 0;
}

int pru_start() {
        int status = pru_set_module_mode(2);
        if (status) {
                printk(KERN_ERR
                       "Failed to set PRU "
                       "module mode to %s\n",
                       status);
                return -1;
        }

        return 0;
}*/

// void pru_stop();

#endif  // _PRU_CTRL_H