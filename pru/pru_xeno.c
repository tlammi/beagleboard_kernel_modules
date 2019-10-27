#include <asm/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <rtdm/driver.h>

#include "pru_ctrl.h"

MODULE_LICENSE("Dual BSD/GPL");

static int pru_open(struct rtdm_fd* fd, int oflags) {
        rtdm_printk(KERN_ALERT "PRU driver opened\n");
        struct pru_context* pctx = rtdm_fd_to_private(fd);

        // Allocate memory regions
        int res = pru_init_context(pctx, PRU_ICSS1);
        if (res) {
                rtdm_printk(KERN_ERR "pru_init_context() failed: %i\n", res);
                return res;
        }

        // Enabled PRU clock domain gating
        printk(KERN_INFO "Enabling PRU-ICSS\n");
        pru_set_device_state_async(pctx, PRU_STATE_ENABLED);

        // Wait for device state
        printk(KERN_INFO "Waiting for PRU-ICSS to reach expected state\n");
        res = pru_wait_for_device_state(pctx, PRU_STATE_ENABLED,
                                        1000 * 1000 * 1000);
        if (res) {
                printk(KERN_ERR
                       "PRU-ICSS did not reach the expected state within "
                       "timeout\n");
                pru_set_device_state_async(pctx, PRU_STATE_DISABLED);
                pru_free_context(pctx);
        } else
                printk(KERN_INFO "PRU-ICSS reached the expected state\n");
        return res;
}

static void pru_close(struct rtdm_fd* fd) {
        rtdm_printk(KERN_ALERT "Closing PRU driver\n");

        struct pru_context* pctx = rtdm_fd_to_private(fd);
        // Disable PRU clock domain gating
        uint32_t tmp = ioread32(pctx->pclk);
        rtdm_printk(KERN_ALERT "Clock register read\n");
        tmp &= ~0x3;
        iowrite32(tmp, pctx->pclk);
        rtdm_printk(KERN_ALERT "Clock register written\n");
        // Free memory regions
        pru_free_context(pctx);
}

static ssize_t pru_read_rt(struct rtdm_fd* fd, void __user* buf, size_t size) {
        struct pru_context* pctx = rtdm_fd_to_private(fd);
        if (!pctx) return -EINVAL;

        void* kbuf = rtdm_malloc(PRUSS_PRU_IRAM_SIZE);
        if (!kbuf) return -ENOMEM;
        memcpy_fromio(kbuf, pctx->piram, PRUSS_PRU_IRAM_SIZE);

        int res =
            rtdm_copy_to_user(fd, buf, kbuf, min(PRUSS_PRU_IRAM_SIZE, size));

        rtdm_free(kbuf);
        if (res) return res;
        return min(PRUSS_PRU_IRAM_SIZE, size);
}
static ssize_t pru_read(struct rtdm_fd* fd, void __user* buf, size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver read called from non-RT context\n");
        return 0;
}

static ssize_t pru_write_rt(struct rtdm_fd* fd, const void __user* buf,
                            size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver write called from RT context\n");
        struct pru_context* pctx = rtdm_fd_to_private(fd);
        void* kbuf = rtdm_malloc(PRUSS_PRU_IRAM_SIZE);
        if (!kbuf) return -ENOMEM;
        int res =
            rtdm_copy_from_user(fd, kbuf, buf, min(size, PRUSS_PRU_IRAM_SIZE));

        if (res) {
                rtdm_free(kbuf);
                return res;
        }

        memcpy_toio(pctx->piram, kbuf, min(size, PRUSS_PRU_IRAM_SIZE));

        rtdm_free(kbuf);
        return min(size, PRUSS_PRU_IRAM_SIZE);
}
static ssize_t pru_write(struct rtdm_fd* fd, const void __user* buf,
                         size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver write called from RT context\n");
        return 0;
}

static int pru_ioctl_rt(struct rtdm_fd* fd, unsigned int request,
                        void __user* arg) {
        return -EPERM;
}

static int pru_ioctl(struct rtdm_fd* fd, unsigned int request,
                     void __user* arg) {
        return -EPERM;
}

static int pru_mmap(struct rtdm_fd* fd, struct vm_area_struct* vma) {
        return -EPERM;
}

struct rtdm_driver pru_driver = {
    .profile_info = RTDM_PROFILE_INFO(pru, RTDM_CLASS_RTIPC, 0, 0),
    .device_flags = RTDM_NAMED_DEVICE,
    .context_size = sizeof(struct pru_context),
    .device_count = 1,
    .ops = {.open = pru_open,
            .close = pru_close,
            .ioctl_rt = pru_ioctl_rt,
            .ioctl_nrt = pru_ioctl,
            .read_rt = pru_read_rt,
            .read_nrt = pru_read,
            .write_rt = pru_write_rt,
            .write_nrt = pru_write,
            .mmap = pru_mmap}};

struct rtdm_device pru_device = {
    .driver = &pru_driver, .label = "pru%d", .device_data = NULL};

int __init pru_init(void) {
        struct resource* res = NULL;
        int ret = -ENOMEM;

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

        rtdm_printk(KERN_INFO "Memory regions requested successfully\n");

        // Set to non-null to indicate that resouces are allocated
        pru_device.device_data = (void*)1;
        ret = rtdm_dev_register(&pru_device);
        if (!ret) return ret;
        rtdm_printk(KERN_ERR "rtdm_printk() failed: %i\n", ret);

        rtdm_printk(KERN_INFO "Releasing memory regions\n");
do_free_cfg:
        release_mem_region(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
do_free_clkctrl:
        release_mem_region(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
do_exit:
        rtdm_printk(KERN_INFO "Memory regions released\n");
        return ret;
}

void __exit pru_exit(void) {
        rtdm_printk(KERN_ALERT "Removing PRU driver\n");
        if (pru_device.device_data) {
                rtdm_printk(KERN_ALERT "Releasing memory regions\n");
                release_mem_region(CM_L4PER2_PRUSS1_CLKCTRL_ADDR, 4);
                release_mem_region(PRUSS1_CFG_REG_ADDR, PRUSS_CFG_REG_SIZE);
                release_mem_region(PRUSS1_PRU0_IRAM_ADDR, PRUSS_PRU_IRAM_SIZE);
                pru_device.device_data = NULL;
        }
        rtdm_printk(KERN_ALERT "Unregistering device\n");
        rtdm_dev_unregister(&pru_device);
        rtdm_printk(KERN_ALERT "Device unregistered\n");
}

module_init(pru_init);
module_exit(pru_exit);