#ifndef _PRU_MAPPING_H
#define _PRU_MAPPING_H

#include <linux/types.h>

#define __REG(type, addr) (*(volatile type* const)(addr))
#define __REGU32(addr) __REG(uint32_t, addr)


// Address definitions
#define PRUSS1_SLAVE_PORT_ADDR 0x4b200000
#define PRUSS1_CFG_ADDR 0x4b226000
#define PRUSS2_CFG_ADDR 0x4b2a6000

#define CM_L4PER2_PRUSS1_CLKCTRL_ADDR 0x4a009718

#define PRUSS1_PRU0_IRAM_ADDR (PRUSS1_SLAVE_PORT_ADDR + 0x34000)

// Register definitions
#define PRUSS1_REVID __REGU32(PRUSS1_CFG_ADDR + 0x0)
#define PRUSS1_SYSCFG __REGU32(PRUSS1_CFG_ADDR + 0x4)
#define PRUSS1_GPCFG0 __REGU32(PRUSS1_CFG_ADDR + 0x8)
#define PRUSS1_GPCFG1 __REGU32(PRUSS1_CFG_ADDR + 0xc)
#define PRUSS1_CGR __REGU32(PRUSS1_CFG_ADDR + 0x10)
#define PRUSS1_ISRP __REGU32(PRUSS1_CFG_ADDR + 0x14)
#define PRUSS1_ISP __REGU32(PRUSS1_CFG_ADDR + 0x18)
#define PRUSS1_IESP __REGU32(PRUSS1_CFG_ADDR + 0x1c)
#define PRUSS1_IECP __REGU32(PRUSS1_CFG_ADDR + 0x20)
#define PRUSS1_PMAO __REGU32(PRUSS1_CFG_ADDR + 0x28)
#define PRUSS1_MII_RT __REGU32(PRUSS1_CFG_ADDR + 0x2c)
#define PRUSS1_IEPCLK __REGU32(PRUSS1_CFG_ADDR + 0x30)
#define PRUSS1_SSP __REGU32(PRUSS1_CFG_ADDR + 0x34)
#define PRUSS1_PIN_MX __REGU32(PRUSS1_CFG_ADDR + 0x40)

// Memory areas
#define PRUSS1_PRU0_IRAM (volatile void* const)(PRUSS1_PRU0_IRAM_ADDR)

#undef __REG
#undef __REGU32
#endif // _PRU_MAPPING_H