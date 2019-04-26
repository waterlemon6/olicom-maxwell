#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sys_config.h>

#define LED1 GPIOH(6)
#define LED2 GPIOH(8) // GPIO(7)
#define EXTI GPIOH(7)

/**
 * 0 - led
 * 1 - exti
 */
#define TRINKETS_NUM 2
static struct class *wl_trinkets;
static dev_t devno;

/**
 * led
 */
struct device *led_device;
static int led_status;

static void led_config(void)
{
	gpio_request(LED1, NULL);
	gpio_request(LED2, NULL);
	gpio_direction_output(LED1, 1);
	gpio_direction_output(LED2, 1);
	led_status = 0;
}

static void led_deconfig(void)
{
	led_status = 0;
	gpio_direction_output(LED1, 1);
	gpio_direction_output(LED2, 1);
	gpio_free(LED1);
	gpio_free(LED2);
}

static void led_control(int i)
{
	switch (i) {
		case 0:
			gpio_direction_output(LED1, 1);
			gpio_direction_output(LED2, 1);
			break;
		case 1:
			gpio_direction_output(LED1, 0);
			gpio_direction_output(LED2, 1);
			break;
		case 2:
			gpio_direction_output(LED1, 1);
			gpio_direction_output(LED2, 0);
			break;
		case 3:
			gpio_direction_output(LED1, 0);
			gpio_direction_output(LED2, 0);
			break;
		default:
			break;
	}
}

static ssize_t show_led_state(struct device *pdev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[led]""Valid input: off(0) 1 2 on(3) toggle.\n"
											"[led]""Current status: %d.\n", led_status);
}

static ssize_t set_led_state(struct device *pdev, struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	if (strncmp(buf, "off", 3) == 0)
		led_status = 0;
	else if (strncmp(buf, "on", 2) == 0)
		led_status = 3;
	else if(strncmp(buf, "toggle", 6) == 0)
		led_status = (~led_status) & 0x03;
	else
	{
		sscanf(buf, "%d", &input);
		if (input & (~0x03))
			printk(KERN_ALERT "[led]""Input error.\n");
		else
			led_status = input & 0x03;
	}

	led_control(led_status);
	return size;
}

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR, show_led_state, set_led_state);

/**
 * exti
 */
struct device *exti_device;
static int exti_count;

static void exti_config(void)
{
	exti_count = 0;
}

static void exti_deconfig(void)
{
	exti_count = 0;
}

/**
 * trinkets
 */
static int trinkets_creat(struct device *dev, dev_t devno, char *name, struct device_attribute *attr)
{
	int status = 0;
	dev = device_create(wl_trinkets, NULL, devno, NULL, name);
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		printk(KERN_ALERT "[trinkets]""Creat device error.\n");
		return status;
	}

	if (attr)
		status = device_create_file(dev, attr);
	if (status)
		printk(KERN_ALERT "[trinkets]""Creat device file error.\n");
	return status;
}

static void trinkets_destroy(struct device *dev, dev_t devno, struct device_attribute *attr)
{
	if (attr)
		device_remove_file(dev, attr);

	device_destroy(wl_trinkets, devno);
}

static int trinkets_init(void)
{
	int status = 0;
	printk(KERN_ALERT "-------trinkets configuration.-------\n");

	wl_trinkets = class_create(THIS_MODULE, "wl_trinkets");
	if (IS_ERR(wl_trinkets)) {
		status = PTR_ERR(wl_trinkets);
		printk(KERN_ALERT "[trinkets]""Create class error.\n");
		return status;
	}

	status = alloc_chrdev_region(&devno, 0, TRINKETS_NUM, "Waterlemon Trinkets");
	if (status) {
		printk(KERN_ALERT "[trinkets]""Allocate chrdev region error.\n");
		class_destroy(wl_trinkets);
		return status;
	}

	status = trinkets_creat(led_device, devno, "led", &dev_attr_state);
	if (!status)
		led_config();

	status = trinkets_creat(exti_device, devno + 1, "exti", NULL);
	if (!status)
		exti_config();

	return 0;
}

static void trinkets_exit(void)
{
	printk(KERN_ALERT "-------trinkets deconfiguration.-------\n");

	led_deconfig();
	trinkets_destroy(led_device, devno, &dev_attr_state);
	exti_deconfig();
	trinkets_destroy(exti_device, devno + 1, NULL);

	unregister_chrdev_region(devno, 2);
	class_destroy(wl_trinkets);
}

module_init(trinkets_init);
module_exit(trinkets_exit);

MODULE_LICENSE("GPL");
