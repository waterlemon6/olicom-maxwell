#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/sys_config.h>

// TDI -> output -> PD21
// TCK -> output -> PD20
// TMS -> output -> PD19
// TDO -> input -> PD18

#define JTAG_GPIO_REG_ADDRESS (0x01C20800 + 0x7C)

static int jtag_init(void);
static void jtag_exit(void);
static int jtag_open(struct inode *inode, struct file *filp);
static int jtag_release(struct inode *inode, struct file *filp);
static long	jtag_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
static int jtag_mmap(struct file *filp, struct vm_area_struct *vma);

static struct file_operations jtag_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = jtag_ioctl,
	.open = jtag_open,
	.release = jtag_release,
	.mmap = jtag_mmap,
};

static struct miscdevice jtag_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "wl_jtag",
	.fops = &jtag_fops,
};

static long	jtag_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	unsigned int tmp;
	switch(_IOC_NR(cmd)) {
		case 0:
			tmp = JTAG_GPIO_REG_ADDRESS - (JTAG_GPIO_REG_ADDRESS & PAGE_MASK);
			__put_user(tmp, (__u32 __user *)arg);
			break;
		default:
			break;
	}
	return 0;
}

static int jtag_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = PAGE_ALIGN(vma->vm_end - vma->vm_start);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return io_remap_pfn_range(vma, vma->vm_start, JTAG_GPIO_REG_ADDRESS >> PAGE_SHIFT, size, vma->vm_page_prot);
}

static int jtag_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[jtag] jtag open.\n");
	return 0;
}

static int jtag_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[jtag] jtag release.\n");
	return 0;
}

static int jtag_init(void)
{
	printk(KERN_ALERT "-------jtag configuration.-------\n");

	gpio_request(GPIOD(18), NULL);
	gpio_request(GPIOD(19), NULL);
	gpio_request(GPIOD(20), NULL);
	gpio_request(GPIOD(21), NULL);
	gpio_direction_input(GPIOD(18));
	gpio_direction_output(GPIOD(19), 0);
	gpio_direction_output(GPIOD(20), 0);
	gpio_direction_output(GPIOD(21), 0);

	misc_register(&jtag_miscdev);

	return 0;
}

static void jtag_exit(void)
{
	printk(KERN_ALERT "-------jtag deconfiguration.-------\n");

	gpio_free(GPIOD(18));
	gpio_free(GPIOD(19));
	gpio_free(GPIOD(20));
	gpio_free(GPIOD(21));

	misc_deregister(&jtag_miscdev);
}

module_init(jtag_init);
module_exit(jtag_exit);

MODULE_LICENSE("GPL");
