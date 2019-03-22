#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/sys_config.h>

static long	led_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
static int led_open(struct inode *inode, struct file *filp);
static int led_release(struct inode *inode, struct file *filp);

static int led_init(void);
static void led_exit(void);

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
	.open = led_open,
	.release = led_release,
};

static struct miscdevice led_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "wl_led",
	.fops = &led_fops,
};

static long	led_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	switch (_IOC_NR(cmd)) {
		case 0:
			gpio_direction_output(GPIOH(6), 1);
			gpio_direction_output(GPIOH(7), 1);
			break;
		case 1:
			gpio_direction_output(GPIOH(6), 0);
			gpio_direction_output(GPIOH(7), 1);
			break;
		case 2:
			gpio_direction_output(GPIOH(6), 1);
			gpio_direction_output(GPIOH(7), 0);
			break;
		case 3:
			gpio_direction_output(GPIOH(6), 0);
			gpio_direction_output(GPIOH(7), 0);
			break;
		default:
			printk(KERN_ALERT "[led] led error cmd.\n");
			break;
	}
	return 0;
}

static int led_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[led] led open.\n");
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[led] led release.\n");
	return 0;
}

static int led_init(void)
{
	printk(KERN_ALERT "-------led configuration.-------\n");

	gpio_request(GPIOH(6), NULL);
	gpio_request(GPIOH(7), NULL);
	gpio_direction_output(GPIOH(6), 1);
	gpio_direction_output(GPIOH(7), 1);

	misc_register(&led_miscdev);

	return 0;
}

static void led_exit(void)
{
	printk(KERN_ALERT "-------led deconfiguration.-------\n");

	gpio_direction_output(GPIOH(6), 1);
	gpio_direction_output(GPIOH(7), 1);
	gpio_free(GPIOH(6));
	gpio_free(GPIOH(7));

	misc_deregister(&led_miscdev);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
