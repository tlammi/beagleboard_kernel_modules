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

        int res = pru_init_context(pctx, PRU_ICSS1);
        if (res) {
                rtdm_printk("pru_init_context() failed: %i\n", res);
                return res;
        }

        return res;
}

static void pru_close(struct rtdm_fd* fd) {
        rtdm_printk(KERN_ALERT "PRU driver closed\n");

        struct pru_context* pctx = rtdm_fd_to_private(fd);

        pru_free_context(pctx, PRU_ICSS1);
}

static ssize_t pru_read_rt(struct rtdm_fd* fd, void __user* buf, size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver read called from RT context\n");
        return size;
}
static ssize_t pru_read(struct rtdm_fd* fd, void __user* buf, size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver read called from non-RT context\n");
        return 0;
}

static ssize_t pru_write_rt(struct rtdm_fd* fd, const void __user* buf,
                            size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver write called from RT context\n");
        return size;
}
static ssize_t pru_write(struct rtdm_fd* fd, const void __user* buf,
                         size_t size) {
        rtdm_printk(KERN_ALERT "PRU driver write called from RT context\n");
        return 0;
}

static int pru_ioctl_rt(struct rtdm_fd* fd, unsigned int request,
                        void __user* arg) {
        return 0;
}

static int pru_ioctl(struct rtdm_fd* fd, unsigned int request,
                     void __user* arg) {
        return 0;
}

struct rtdm_driver pru_driver = {
    .profile_info = RTDM_PROFILE_INFO(pru, RTDM_CLASS_RTIPC, 0, 0),
    .device_flags = RTDM_NAMED_DEVICE,
    .context_size = sizeof(struct pru_context),
    .device_count = 1,
    .ops = {
        .open = pru_open,
        .close = pru_close,
        .ioctl_rt = pru_ioctl_rt,
        .ioctl_nrt = pru_ioctl,
        .read_rt = pru_read_rt,
        .read_nrt = pru_read,
        .write_rt = pru_write_rt,
        .write_nrt = pru_write,
    }};

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