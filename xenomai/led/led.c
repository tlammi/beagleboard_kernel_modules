

#include <linux/module.h>
#include <linux/ioport.h>
#include <rtdm/driver.h>


static const unsigned long LED_DATAOUT_REG = 0x4805113c;
static const unsigned long LED_OE_REG = 0x48051134;
static const unsigned LED_CTRL_OFFSET = 0x13c;
static int LED_CNTRL_BIT = 14;

struct led_context {
    struct resource* p_ctrl_region;
    struct resource* p_oe_region;
    void* p_ctrl;
    void* p_oe;
};

/*
static void led_thread(void* pargs){

    struct led_context* pctx = (struct led_context*)pargs;

    const int64_t SECOND = 1000000000;
    rtdm_task_set_period(&pctx->task, 0, SECOND);

    while(1){
        rtdm_task_wait_period(NULL);
        rtdm_printk(KERN_ALERT "print from task\n");
    }
}
*/

static int led_open(struct rtdm_fd* fd, int oflags){
    rtdm_printk(KERN_ALERT "Opening led device\n");
    struct led_context *pctx = (struct led_context*) rtdm_fd_to_private(fd);
    pctx->p_ctrl_region = request_mem_region(LED_DATAOUT_REG, 4, "USER2_LED");
    if(!pctx->p_ctrl_region){
        return -EACCES;
    }

    pctx->p_oe_region = request_mem_region(LED_OE_REG, 4, "USER2_LED");
    if(!pctx->p_oe_region){
        release_mem_region(pctx->p_ctrl_region->start, 4);
        pctx->p_ctrl_region = NULL;
        return -EACCES;
    }

    pctx->p_ctrl = ioremap(LED_DATAOUT_REG, 4);

    if(! pctx->p_ctrl){
        release_mem_region(pctx->p_ctrl_region->start, 4);
        release_mem_region(pctx->p_oe_region->start, 4);

        pctx->p_ctrl_region = NULL;
        pctx->p_oe_region = NULL;

        return -EACCES;
    }

    pctx->p_oe = ioremap(LED_OE_REG, 4);

    if(! pctx->p_oe){
        release_mem_region(pctx->p_ctrl_region->start, 4);
        release_mem_region(pctx->p_oe_region->start, 4);
        iounmap(pctx->p_ctrl);

        pctx->p_ctrl_region = NULL;
        pctx->p_oe_region = NULL;
        pctx->p_ctrl = NULL;
        return -EACCES;
    }

    iowrite32(0, pctx->p_oe);
    rtdm_printk(KERN_ALERT "led device opened successfully\n");
    return 0;
}

static void led_close(struct rtdm_fd* fd){
    rtdm_printk(KERN_ALERT "Closing led device\n");

    struct led_context *pctx = (struct led_context*) rtdm_fd_to_private(fd);

    if(pctx->p_ctrl){
        iounmap(pctx->p_ctrl);
        pctx->p_ctrl = NULL;
    }

    if(pctx->p_oe){
        iounmap(pctx->p_oe);
        pctx->p_oe = NULL;
    }

    if(pctx->p_ctrl_region){
        release_mem_region(pctx->p_ctrl_region->start, 4);
        pctx->p_ctrl_region = NULL;
    }

    if(pctx->p_oe_region){
        release_mem_region(pctx->p_oe_region->start, 4);
        pctx->p_oe_region = NULL;
    }

    rtdm_printk(KERN_ALERT "led device closed\n");
}

static ssize_t led_read(struct rtdm_fd *fd, void __user *buf, size_t size){
    int i=0;
    static char kernel_buf[100];
    rtdm_printk(KERN_ALERT "Driver read called}\n");
    rtdm_printk(KERN_ALERT "Buffer size: %d\n", size);

    for(; i < size; i++){
        kernel_buf[i] = i;
    }

    if(rtdm_copy_to_user(fd, buf, kernel_buf, min(100, size))){
        rtdm_printk(KERN_ALERT "Failed to copy to user\n");
        return -EIO;
    }
    return size;
}

static ssize_t led_write(struct rtdm_fd *fd, const void __user *buf, size_t size){
    struct led_context *pctx = rtdm_fd_to_private(fd);

    if(size != 4){
        return -EPERM;
    }

    uint32_t write_value = 0;

    int copy_result = 0;
    if(!(copy_result = rtdm_copy_from_user(fd, &write_value, buf, size))){
        return copy_result;
    }

    iowrite32(write_value, pctx->p_ctrl);

    return size;
}

static int led_ioctl(struct rtdm_fd *fd, unsigned int request, void __user *arg){
    rtdm_printk(KERN_ALERT "Driver ioctl called\n");
    return EPERM;
}

struct rtdm_driver led_driver = {
    .profile_info = RTDM_PROFILE_INFO(led, RTDM_CLASS_GPIO, 0, 0),
    .device_flags = RTDM_NAMED_DEVICE,
    .context_size = sizeof(struct led_context),
    .device_count = 1,
    .ops = {
        .open = led_open,
        .close = led_close,
        .ioctl_rt = led_ioctl,
        .ioctl_nrt = led_ioctl,
        .read_rt = led_read,
        .read_nrt = led_read,
        .write_rt = led_write,
        .write_nrt = led_write,
    }
};

struct rtdm_device led_device = {
    .driver = &led_driver,
    .label = "led%d"
};


static int __init led_init(void){
    int retval;
    rtdm_printk(KERN_ALERT "Hello, Xenomai kernel\n");
    if((retval = rtdm_dev_register(&led_device))){
        rtdm_printk(KERN_ALERT "rtdm_dev_register failed: %d\n", retval);
    }

    return retval;
}


static void __exit led_exit(void){
    rtdm_printk(KERN_ALERT "Goodbye, cruel world\n");
    rtdm_dev_unregister(&led_device);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
