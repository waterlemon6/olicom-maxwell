#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sys_config.h>

#define LED1 GPIOH(6)
#define LED2 GPIOH(7)

static struct class *led_class;
static dev_t led_devno;
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

static int led_init(void)
{
	int status = 0;
	printk(KERN_ALERT "-------led configuration.-------\n");

	led_class = class_create(THIS_MODULE, "wl_led");
	if (IS_ERR(led_class)) {
		status = PTR_ERR(led_class);
		printk(KERN_ALERT "[led]""Create class error.\n");
		return status;
	}

	status = alloc_chrdev_region(&led_devno, 0, 1, "Waterlemon LED");
	if (status) {
		printk(KERN_ALERT "[led]""Allocate chrdev region error.\n");
		class_destroy(led_class);
		return status;
	}

	led_device = device_create(led_class, NULL, led_devno, NULL, "led");
	if (IS_ERR(led_device)) {
		status = PTR_ERR(led_device);
		printk(KERN_ALERT "[led]""Creat device error.\n");
		unregister_chrdev_region(led_devno, 1);
		class_destroy(led_class);
		return status;
	}

	status = device_create_file(led_device, &dev_attr_state);
	if (status) {
		printk(KERN_ALERT "[led]""Creat device file error.\n");
		device_destroy(led_class, led_devno);
		unregister_chrdev_region(led_devno, 1);
		class_destroy(led_class);
		return status;
	}

	led_config();
	return 0;
}

static void led_exit(void)
{
	printk(KERN_ALERT "-------led deconfiguration.-------\n");

	led_deconfig();
	device_remove_file(led_device, &dev_attr_state);
	device_destroy(led_class, led_devno);
	unregister_chrdev_region(led_devno, 1);
	class_destroy(led_class);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
