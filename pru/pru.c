#include <linux/init.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <asm/errno.h>

MODULE_LICENSE("Dual BSD/GPL");

#define PRU_MEM_BASE 0x4B200000
#define PRU_MEM_END 0x4B27FFFF
#define PRU_MEM_LEN ((PRU_MEM_END) - (PRU_MEM_BASE) + 1)

static void* __iomem ppru_iomem;

void* allocate_pru(void){
    printk(KERN_INFO "Allocating %i bytes starting from address %i\n", PRU_MEM_LEN, PRU_MEM_BASE);
    struct resource* reg = request_mem_region(PRU_MEM_BASE, PRU_MEM_LEN, "PRU-ICSS1");
    if(reg == NULL){
        printk(KERN_ERR "Failed to request memory region\n");
        return NULL;
    }

    void* pmem = ioremap(PRU_MEM_BASE, PRU_MEM_LEN);

    if(pmem == NULL){
        printk(KERN_ERR "Failed to map IO memory\n");
        release_mem_region(PRU_MEM_BASE, PRU_MEM_LEN);
    }
    else{
        printk(KERN_INFO "Memory allocated successfully\n");
    }
    return pmem;
}

void deallocate_pru(void* pmem){
    if(pmem == NULL) return;
    iounmap(pmem);
    release_mem_region(PRU_MEM_BASE, PRU_MEM_LEN);
}


static int __init init_pru(void){
    printk(KERN_INFO "Loading PRU driver module\n");
    ppru_iomem = allocate_pru();
    if(ppru_iomem == NULL){
        return -EIO;
    }

exit_success:
    printk(KERN_INFO "PRU driver module loaded successfully\n");
    return 0;
}



void __exit exit_pru(void){
    printk(KERN_INFO "Unloading PRU driver module\n");
    deallocate_pru(ppru_iomem);
    ppru_iomem = NULL;
    printk(KERN_INFO "PRU driver unloaded successfully\n");
}

module_init(init_pru);
module_exit(exit_pru);