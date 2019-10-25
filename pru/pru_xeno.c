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
                rtdm_printk(KERN_INFO "pru_init_context() failed: %i\n", res);
                return res;
        }
        // Enable PRU clock domain gating
        rtdm_printk(KERN_ALERT "Enabling PRU-ICSS1/PRU0\n");
        uint32_t tmp = ioread32(pctx->clkctrl.pmap);
        rtdm_printk(KERN_ALERT "Clock control register has value %08x\n", tmp);
        tmp &= ~0x3;
        tmp |= 2;
        rtdm_printk(KERN_ALERT "Clock control register value to write: %08x\n",
                    tmp);
        iowrite32(tmp, pctx->clkctrl.pmap);
        int counter = 10000;
        do {
                tmp = ioread32(pctx->clkctrl.pmap);
                counter--;
        } while (((tmp & (0x3 << 16)) == (3 << 16)) && counter);

        if (!counter) {
                rtdm_printk(KERN_ALERT
                            "Clock control register left in state: %08x\n",
                            tmp);
                return -EIO;
        }
        rtdm_printk(KERN_ALERT
                    "Clock control register written. New value: %08x\n",
                    tmp);
        return res;
}

static void pru_close(struct rtdm_fd* fd) {
        rtdm_printk(KERN_ALERT "PRU driver closed\n");

        struct pru_context* pctx = rtdm_fd_to_private(fd);
        // Disable PRU clock domain gating
        uint32_t tmp = ioread32(pctx->clkctrl.pmap);
        tmp &= ~0x3;
        iowrite32(tmp, pctx->clkctrl.pmap);
        // Free memory regions
        pru_free_context(pctx, PRU_ICSS1);
}

static ssize_t pru_read_rt(struct rtdm_fd* fd, void __user* buf, size_t size) {
        struct pru_context* pctx = rtdm_fd_to_private(fd);
        if (!pctx) return -EINVAL;

        void* kbuf = rtdm_malloc(PRUSS_PRU_IRAM_SIZE);
        if (!kbuf) return -ENOMEM;
        memcpy_fromio(kbuf, pctx->iram.pmap, PRUSS_PRU_IRAM_SIZE);

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

        memcpy_toio(pctx->iram.pmap, kbuf, min(size, PRUSS_PRU_IRAM_SIZE));

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

struct rtdm_device pru_device = {.driver = &pru_driver, .label = "pru%d"};
int __init pru_init(void) {
        int res = 0;
        rtdm_printk(KERN_ALERT "Hello, Xenomai kernel\n");
        if (res = rtdm_dev_register(&pru_device)) {
                rtdm_printk(KERN_ERR "rtdm_printk() failed: %i\n", res);
        }
        return res;
}

void __exit pru_exit(void) {
        rtdm_printk(KERN_ALERT "Goodbye, Xenomai kernel\n");
        rtdm_dev_unregister(&pru_device);
}

module_init(pru_init);
module_exit(pru_exit);