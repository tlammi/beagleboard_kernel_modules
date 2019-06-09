

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/errno.h>

MODULE_LICENSE("Dual BSD/GPL");

unsigned long LED_CTRL_REG = 0x4805113c;
unsigned long LED_OE_REG = 0x48051134;
static unsigned LED_CTRL_OFFSET = 0x13c;
int LED_CNTRL_BIT = 14;


static void* __iomem led_base;


static int led_probe(struct platform_device* pdev){

	int retval= -EIO;
	struct resource* iomem_range;

	iomem_range = platform_get_resource(pdev, IORESOURCE_MEM, 0);


	led_base = devm_ioremap_resource(&pdev->dev, iomem_range);

	if(IS_ERR(led_base)){
		printk(KERN_ALERT "Error: devm_ioremap_resource\n");
		retval = PTR_ERR(led_base);
		goto func_exit;
	}

	retval = 0;
	printk(KERN_ALERT"led_probe success\n");
func_exit:
	return retval;
}

static int __init led_init(void){
	int retval = -EIO;
	printk(KERN_ALERT "Requesting memory region\n");
	struct resource* led_ctrl_reg_region = request_mem_region(LED_CTRL_REG, 4, "USER2_LED");
	if(led_ctrl_reg_region == NULL){
	    printk(KERN_ALERT "Memory region request failed\n");
	    goto error;
	}

	struct resource* led_oe_reg_region = request_mem_region(LED_OE_REG, 4, "USER2_LED");
	if(led_oe_reg_region == NULL){
		printk(KERN_ALERT "Memory region 2 request failed\n");
		goto do_free_ctrl_reg;
	}

	printk("Mapping IO memory\n");
	void* oe_ptr = ioremap(LED_OE_REG, 4);
	if(oe_ptr == NULL){
		printk(KERN_ALERT "Failed to map led ctrl memory");
		goto do_free_oe_reg;
	}
	void* ctrl_ptr = ioremap(LED_CTRL_REG, 4);
	if(ctrl_ptr == NULL){
	    printk(KERN_ALERT "Failed to map IO memory\n");
	    goto do_iounmap_oe_ptr;
	}
	printk(KERN_ALERT "Successfully claimed mem region\n");

	unsigned int val = ioread32(ctrl_ptr);
	printk(KERN_ALERT "LED value before write: %i\n", val);
	printk(KERN_ALERT "Setting USER LED\n");

	iowrite32(0, oe_ptr);
	iowrite32(val | 0xffffffff, ctrl_ptr);
	//iowrite32(val & 0, ptr);
	val = ioread32(ctrl_ptr);
	printk(KERN_ALERT "LED value after write: %i\n", val);

	retval = 0;

do_iounmap_ctrl_ptr:
	iounmap(ctrl_ptr);
do_iounmap_oe_ptr:
	iounmap(oe_ptr);
do_free_oe_reg:
	release_mem_region(led_oe_reg_region->start, 4);
do_free_ctrl_reg:
	release_mem_region(led_ctrl_reg_region->start, 4);
error:
	return retval;

}

static void __exit led_exit(void){
	printk(KERN_ALERT "Unregistering led driver\n");
}

module_init(led_init);
module_exit(led_exit);
