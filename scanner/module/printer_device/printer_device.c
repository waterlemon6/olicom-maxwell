#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/usb/composite.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define PRINTER_NOT_ERROR 0x08
#define PRINTER_SELECTED 0x10
#define PRINTER_PAPER_EMPTY 0x20

#define GET_PRINTER_STATUS _IOR('w', 0x21, unsigned char)
#define SET_PRINTER_STATUS _IOWR('w', 0x22, unsigned char)

#define PRINTER_VENDOR_NUM 0x0525 /* NetChip */
#define PRINTER_PRODUCT_NUM 0xa4a8 /* Linux-USB Printer Gadget */
#define USB_BUFSIZE 8192
#define ep_desc(g, hs, fs) (((g)->speed == USB_SPEED_HIGH) ? (hs) : (fs))

struct printer_dev {
	spinlock_t lock; /* lock this structure */
	struct mutex lock_printer_io; /* lock buffer lists during read/write calls */
	struct usb_gadget *gadget;
	signed char interface;
	struct usb_ep *in_ep, *out_ep;

	struct list_head rx_reqs;	/* List of free RX structs */
	struct list_head rx_reqs_active; /* List of Active RX xfers */
	struct list_head rx_buffers; /* List of completed xfers */
	wait_queue_head_t	rx_wait; /* wait until there is data to be read. */

	struct list_head tx_reqs;	/* List of free TX structs */
	struct list_head tx_reqs_active; /* List of Active TX xfers */
	wait_queue_head_t	tx_wait; /* Wait until there are write buffers available to use. */
	wait_queue_head_t	tx_flush_wait; /* Wait until all write buffers have been sent. */

	struct usb_request *current_rx_req;
	size_t current_rx_bytes;
	unsigned char *current_rx_buf;

	unsigned char printer_status;
	unsigned char reset_printer;
	unsigned char printer_cdev_open;
	struct cdev printer_cdev;
	struct device *pdev;
	struct usb_function	function;
};

static DEFINE_MUTEX(printer_mutex);
static struct printer_dev usb_printer_dev;
static dev_t usb_printer_devno;
static struct class *usb_printer_class;
USB_GADGET_COMPOSITE_OPTIONS();

static char *iPNPstring;
module_param(iPNPstring, charp, S_IRUGO);
static unsigned qlen = 10;
module_param(qlen, uint, S_IRUGO|S_IWUSR);

/***********************************************************************
 *													DESCRIPTORS
 ***********************************************************************/
static struct usb_device_descriptor device_desc = {
	.bLength = sizeof device_desc,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.idVendor = cpu_to_le16(PRINTER_VENDOR_NUM),
	.idProduct = cpu_to_le16(PRINTER_PRODUCT_NUM),
	.bNumConfigurations =	1,
};

static struct usb_configuration printer_cfg_driver = {
	.label = "printer",
	.bConfigurationValue = 1,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_interface_descriptor intf_desc = {
	.bLength = sizeof intf_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_PRINTER,
	.bInterfaceSubClass =	1,
	.bInterfaceProtocol =	2,
	.iInterface = 0
};

static struct usb_endpoint_descriptor fs_ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK
};

static struct usb_endpoint_descriptor fs_ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK
};

static struct usb_descriptor_header *fs_printer_function[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &fs_ep_in_desc,
	(struct usb_descriptor_header *) &fs_ep_out_desc,
	NULL
};

static struct usb_endpoint_descriptor hs_ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512)
};

static struct usb_endpoint_descriptor hs_ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512)
};

static struct usb_qualifier_descriptor dev_qualifier = {
	.bLength = sizeof dev_qualifier,
	.bDescriptorType = USB_DT_DEVICE_QUALIFIER,
	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_PRINTER,
	.bNumConfigurations =	1
};

static struct usb_descriptor_header *hs_printer_function[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &hs_ep_in_desc,
	(struct usb_descriptor_header *) &hs_ep_out_desc,
	NULL
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength = sizeof otg_descriptor,
	.bDescriptorType = USB_DT_OTG,
	.bmAttributes = USB_OTG_SRP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

static struct usb_string strings [] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "MAN",
	[USB_GADGET_PRODUCT_IDX].s = "PRO",
	[USB_GADGET_SERIAL_IDX].s =	"1",
	{  }		/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static char pnp_string [1024] = "XXMFG:linux;MDL:wl_printer;CLS:PRINTER;SN:1;";

/***********************************************************************
 *													FILE OPERATIONS
 ***********************************************************************/
static void rx_complete(struct usb_ep *ep, struct usb_request *req)
{
 	struct printer_dev *dev = ep->driver_data;
 	int status = req->status;
 	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_del_init(&req->list);	/* Remode from Active List */
	switch (status) {
		case 0:
			if (req->actual > 0) {
				list_add_tail(&req->list, &dev->rx_buffers);
				//printk(KERN_ALERT "[printer device]""rx length %d\n", req->actual);
			}
			else {
				printk(KERN_ALERT "[printer device]""rx length zero");
				list_add(&req->list, &dev->rx_reqs);
			}
			break;

		case -ECONNRESET:		/* unlink */
		case -ESHUTDOWN:		/* disconnect etc */
			printk(KERN_ALERT "[printer device]""rx shutdown, code %d\n", status);
			list_add(&req->list, &dev->rx_reqs);
			break;

		case -ECONNABORTED:		/* endpoint reset */
			printk(KERN_ALERT "[printer device]""rx %s reset\n", ep->name);
			list_add(&req->list, &dev->rx_reqs);
			break;

		case -EOVERFLOW:
		default:
			printk(KERN_ALERT "[printer device]""rx status %d\n", status);
			list_add(&req->list, &dev->rx_reqs);
			break;
	}
	wake_up_interruptible(&dev->rx_wait);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void setup_rx_reqs(struct printer_dev *dev)
{
	struct usb_request *req;

	if (dev->interface < 0)
		return;

	while (likely(!list_empty(&dev->rx_reqs))) {
 		int error;

 		req = container_of(dev->rx_reqs.next, struct usb_request, list);
 		list_del_init(&req->list);

 		/* The USB Host sends us whatever amount of data it wants to
 		 * so we always set the length field to the full USB_BUFSIZE.
 		 * If the amount of data is more than the read() caller asked
 		 * for it will be stored in the request buffer until it is
 		 * asked for by read().
 		 */
 		req->length = USB_BUFSIZE;
 		req->complete = rx_complete;

		/* here, we unlock, and only unlock, to avoid deadlock. */
		spin_unlock(&dev->lock);
		error = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC);
		spin_lock(&dev->lock);
 		if (error) {
 			printk(KERN_ALERT "[printer device]""rx submit --> %d, interface: %d.\n", error, dev->interface);
 			list_add(&req->list, &dev->rx_reqs);
 			break;
 		}
		/* if the req is empty, then add it into dev->rx_reqs_active. */
		else if (list_empty(&req->list))
			list_add(&req->list, &dev->rx_reqs_active);
	}
}

static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct printer_dev	*dev = ep->driver_data;

	switch (req->status) {
	default:
		printk(KERN_ALERT "[printer device]""tx err %d\n", req->status);
		/* FALLTHROUGH */
	case -ECONNRESET:		/* unlink */
	case -ESHUTDOWN:		/* disconnect etc */
		break;
	case 0:
		break;
	}

	spin_lock(&dev->lock);
	/* Take the request struct off the active list and put it on the
	 * free list.
	 */
	list_del_init(&req->list);
	list_add(&req->list, &dev->tx_reqs);
	wake_up_interruptible(&dev->tx_wait);
	if (likely(list_empty(&dev->tx_reqs_active)))
		wake_up_interruptible(&dev->tx_flush_wait);
	spin_unlock(&dev->lock);
}

static int printer_open(struct inode *inode, struct file *fd)
{
	struct printer_dev *dev = container_of(inode->i_cdev, struct printer_dev, printer_cdev);
	unsigned long flags;
	int ret = -EBUSY;

	spin_lock_irqsave(&dev->lock, flags);
	if (!dev->printer_cdev_open) {
		dev->printer_cdev_open = 1;
		fd->private_data = dev;
		ret = 0;
		/* Change the printer status to show that it's on-line. */
		dev->printer_status |= PRINTER_SELECTED;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return ret;
}

static int printer_close(struct inode *inode, struct file *fd)
{
	struct printer_dev *dev = fd->private_data;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	dev->printer_cdev_open = 0;
	fd->private_data = NULL;
	/* Change printer status to show that the printer is off-line. */
	dev->printer_status &= ~PRINTER_SELECTED;
	spin_unlock_irqrestore(&dev->lock, flags);
 	return 0;
 }

static long printer_ioctl(struct file *fd, unsigned int code, unsigned long arg)
{
	struct printer_dev *dev = fd->private_data;
	unsigned long flags;
	int status = 0;

	spin_lock_irqsave(&dev->lock, flags);
	switch (code) {
		case GET_PRINTER_STATUS:
			status = (int)dev->printer_status;
			break;
		case SET_PRINTER_STATUS:
			dev->printer_status = (unsigned char)arg;
			break;
		default:
			status = -ENOTTY;
			break;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return status;
}

static ssize_t printer_read(struct file *fd, char __user *buf, size_t len, loff_t *ptr)
{
	struct printer_dev *dev = fd->private_data;
	unsigned long flags;
	size_t size;
	size_t bytes_copied;
	struct usb_request *req;
	/* This is a pointer to the current USB rx request. */
	struct usb_request *current_rx_req;
	/* This is the number of bytes in the current rx buffer. */
	size_t current_rx_bytes;
	/* This is a pointer to the current rx buffer. */
	unsigned char *current_rx_buf;

	if (len == 0)
		return -EINVAL;
	//printk(KERN_ALERT "[printer device]""printer_read trying to read %d bytes\n", (int)len);
	mutex_lock(&dev->lock_printer_io);
	spin_lock_irqsave(&dev->lock, flags);

	/* We will use this flag later to check if a printer reset happened
	 * after we turn interrupts back on.
	 */
	dev->reset_printer = 0;

	setup_rx_reqs(dev);
	bytes_copied = 0;
	current_rx_req = dev->current_rx_req;
	current_rx_bytes = dev->current_rx_bytes;
	current_rx_buf = dev->current_rx_buf;
	dev->current_rx_req = NULL;
	dev->current_rx_bytes = 0;
	dev->current_rx_buf = NULL;

	/* Check if there is any data in the read buffers. Please note that
	 * current_rx_bytes is the number of bytes in the current rx buffer.
	 * If it is zero then check if there are any other rx_buffers that
	 * are on the completed list. We are only out of data if all rx
	 * buffers are empty.
	 */
	if ((current_rx_bytes == 0) && (likely(list_empty(&dev->rx_buffers)))) {
		/* Turn interrupts back on before sleeping. */
		spin_unlock_irqrestore(&dev->lock, flags);

		/*
		 * If no data is available check if this is a NON-Blocking
		 * call or not.
		 */
		if (fd->f_flags & (O_NONBLOCK|O_NDELAY)) {
			mutex_unlock(&dev->lock_printer_io);
			return -EAGAIN;
		}

		/* Sleep until data is available */
		wait_event_interruptible(dev->rx_wait, (likely(!list_empty(&dev->rx_buffers))));
		spin_lock_irqsave(&dev->lock, flags);
	}

	/* We have data to return then copy it to the caller's buffer.*/
	while ((current_rx_bytes || likely(!list_empty(&dev->rx_buffers))) && len) {
		if (current_rx_bytes == 0) {
			req = container_of(dev->rx_buffers.next, struct usb_request, list);
			list_del_init(&req->list);

			if (req->actual && req->buf) {
				current_rx_req = req;
				current_rx_bytes = req->actual;
				current_rx_buf = req->buf;
			}
			else {
				list_add(&req->list, &dev->rx_reqs);
				continue;
			}
		}

		/* Don't leave irqs off while doing memory copies */
		spin_unlock_irqrestore(&dev->lock, flags);

		if (len > current_rx_bytes)
			size = current_rx_bytes;
		else
			size = len;

		size -= copy_to_user(buf, current_rx_buf, size);
		bytes_copied += size;
		len -= size;
		buf += size;

		spin_lock_irqsave(&dev->lock, flags);

		/* We've disconnected or reset so return. */
		if (dev->reset_printer) {
			list_add(&current_rx_req->list, &dev->rx_reqs);
			spin_unlock_irqrestore(&dev->lock, flags);
			mutex_unlock(&dev->lock_printer_io);
			return -EAGAIN;
		}

		/* If we not returning all the data left in this RX request
		 * buffer then adjust the amount of data left in the buffer.
		 * Othewise if we are done with this RX request buffer then
		 * requeue it to get any incoming data from the USB host.
		 */
		if (size < current_rx_bytes) {
			current_rx_bytes -= size;
			current_rx_buf += size;
		}
		else {
			list_add(&current_rx_req->list, &dev->rx_reqs);
			current_rx_bytes = 0;
			current_rx_buf = NULL;
			current_rx_req = NULL;
		}
	}

	dev->current_rx_req = current_rx_req;
	dev->current_rx_bytes = current_rx_bytes;
	dev->current_rx_buf = current_rx_buf;

	spin_unlock_irqrestore(&dev->lock, flags);
	mutex_unlock(&dev->lock_printer_io);
	//printk(KERN_ALERT "[printer device]""printer_read returned %d bytes\n", (int)bytes_copied);

	if (bytes_copied)
		return bytes_copied;
	else
		return -EAGAIN;
}

static ssize_t printer_write(struct file *fd, const char __user *buf, size_t len, loff_t *ptr)
{
	struct printer_dev *dev = fd->private_data;
	unsigned long flags;
	size_t size;	/* Amount of data in a TX request. */
	size_t bytes_copied = 0;
	struct usb_request *req;
	int value;

	//printk(KERN_ALERT "[printer device]""printer_write trying to send %d bytes\n", (int)len);
	if (len == 0)
		return -EINVAL;
	mutex_lock(&dev->lock_printer_io);
	spin_lock_irqsave(&dev->lock, flags);

	/* Check if a printer reset happens while we have interrupts on */
	dev->reset_printer = 0;

	/* Check if there is any available write buffers */
	if (likely(list_empty(&dev->tx_reqs))) {
		/* Turn interrupts back on before sleeping. */
		spin_unlock_irqrestore(&dev->lock, flags);

		/*
		 * If write buffers are available check if this is
		 * a NON-Blocking call or not.
		 */
		if (fd->f_flags & (O_NONBLOCK|O_NDELAY)) {
			mutex_unlock(&dev->lock_printer_io);
			return -EAGAIN;
		}

		/* Sleep until a write buffer is available */
		wait_event_interruptible(dev->tx_wait, (likely(!list_empty(&dev->tx_reqs))));
		spin_lock_irqsave(&dev->lock, flags);
	}

	while (likely(!list_empty(&dev->tx_reqs)) && len) {
		if (len > USB_BUFSIZE)
			size = USB_BUFSIZE;
		else
			size = len;

		req = container_of(dev->tx_reqs.next, struct usb_request, list);
		list_del_init(&req->list);

		req->complete = tx_complete;
		req->length = size;

		/* Check if we need to send a zero length packet. */
		if (len > size)
			/* They will be more TX requests so no yet. */
			req->zero = 0;
		else
			/* If the data amount is not a multple of the
			 * maxpacket size then send a zero length packet.
			 */
			req->zero = 0;
			//req->zero = ((len % dev->in_ep->maxpacket) == 0);

		/* Don't leave irqs off while doing memory copies */
		spin_unlock_irqrestore(&dev->lock, flags);

		if (copy_from_user(req->buf, buf, size)) {
			list_add(&req->list, &dev->tx_reqs);
			mutex_unlock(&dev->lock_printer_io);
			return bytes_copied;
		}

		bytes_copied += size;
		len -= size;
		buf += size;

		spin_lock_irqsave(&dev->lock, flags);

		/* We've disconnected or reset so free the req and buffer */
		if (dev->reset_printer) {
			list_add(&req->list, &dev->tx_reqs);
			spin_unlock_irqrestore(&dev->lock, flags);
			mutex_unlock(&dev->lock_printer_io);
			return -EAGAIN;
		}

		list_add(&req->list, &dev->tx_reqs_active);

		/* here, we unlock, and only unlock, to avoid deadlock. */
		spin_unlock(&dev->lock);
		value = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
		spin_lock(&dev->lock);
		if (value) {
			list_del(&req->list);
			list_add(&req->list, &dev->tx_reqs);
			spin_unlock_irqrestore(&dev->lock, flags);
			mutex_unlock(&dev->lock_printer_io);
			return -EAGAIN;
		}
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	mutex_unlock(&dev->lock_printer_io);
	//printk(KERN_ALERT "[printer device]""printer_write sent %d bytes\n", (int)bytes_copied);

	if (bytes_copied) {
		return bytes_copied;
	} else {
		return -EAGAIN;
	}
}

static int printer_fsync(struct file *fd, loff_t start, loff_t end, int datasync)
{
	struct printer_dev	*dev = fd->private_data;
	struct inode *inode = file_inode(fd);
	unsigned long flags;
	int tx_list_empty;

	mutex_lock(&inode->i_mutex);
	spin_lock_irqsave(&dev->lock, flags);
	tx_list_empty = (likely(list_empty(&dev->tx_reqs_active)));
	spin_unlock_irqrestore(&dev->lock, flags);

	/* Sleep until all data has been sent */
	if (!tx_list_empty)
		wait_event_interruptible(dev->tx_flush_wait, (likely(list_empty(&dev->tx_reqs_active))));
	mutex_unlock(&inode->i_mutex);
	return 0;
}

static unsigned int printer_poll(struct file *fd, poll_table *wait)
{
	struct printer_dev *dev = fd->private_data;
	unsigned long flags;
	int status = 0;

	mutex_lock(&dev->lock_printer_io);
	spin_lock_irqsave(&dev->lock, flags);
	setup_rx_reqs(dev);
	spin_unlock_irqrestore(&dev->lock, flags);
	mutex_unlock(&dev->lock_printer_io);

	poll_wait(fd, &dev->rx_wait, wait);
	poll_wait(fd, &dev->tx_wait, wait);

	spin_lock_irqsave(&dev->lock, flags);
	if (likely(!list_empty(&dev->tx_reqs)))
		status |= POLLOUT | POLLWRNORM;
	if (likely(dev->current_rx_bytes) || likely(!list_empty(&dev->rx_buffers)))
		status |= POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&dev->lock, flags);
	return status;
}

static const struct file_operations printer_io_operations = {
	.owner = THIS_MODULE,
	.open = printer_open,
	.release = printer_close,
	.unlocked_ioctl = printer_ioctl,
	.read = printer_read,
	.write =	printer_write,
	.fsync =	printer_fsync,
	.poll =		printer_poll,
};
/***********************************************************************
 *													USB SUB PROCESS
 ***********************************************************************/
static struct usb_request *printer_req_alloc(struct usb_ep *ep, unsigned len, gfp_t gfp_flags)
{
	struct usb_request *req = usb_ep_alloc_request(ep, gfp_flags);

	if (req != NULL) {
		req->length = len;
		req->buf = kmalloc(len, gfp_flags);
		if (req->buf == NULL) {
			usb_ep_free_request(ep, req);
			return NULL;
		}
	}
	return req;
}

static void printer_req_free(struct usb_ep *ep, struct usb_request *req)
{
	if (ep != NULL && req != NULL) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static int printer_set_new_interface(struct printer_dev *dev)
{
	int result = 0;

	dev->in_ep->desc = ep_desc(dev->gadget, &hs_ep_in_desc, &fs_ep_in_desc);
	dev->in_ep->driver_data = dev;

	dev->out_ep->desc = ep_desc(dev->gadget, &hs_ep_out_desc, &fs_ep_out_desc);
	dev->out_ep->driver_data = dev;

	result = usb_ep_enable(dev->in_ep);
	if (result != 0) {
		printk(KERN_ALERT "[printer device]""enable %s --> %d\n", dev->in_ep->name, result);
		goto done;
	}

	result = usb_ep_enable(dev->out_ep);
	if (result != 0) {
		printk(KERN_ALERT "[printer device]""enable %s --> %d\n", dev->in_ep->name, result);
		goto done;
	}

done:
	/* on error, disable any endpoints  */
	if (result != 0) {
		(void) usb_ep_disable(dev->in_ep);
		(void) usb_ep_disable(dev->out_ep);
		dev->in_ep->desc = NULL;
		dev->out_ep->desc = NULL;
	}

	/* caller is responsible for cleanup on error */
	return result;
}

static void printer_reset_interface(struct printer_dev *dev)
{
	unsigned long	flags;
	if (dev->interface < 0)
		return;

	printk(KERN_ALERT "[printer device]""%s\n", __func__);
	if (dev->in_ep->desc)
		usb_ep_disable(dev->in_ep);
	if (dev->out_ep->desc)
		usb_ep_disable(dev->out_ep);

	spin_lock_irqsave(&dev->lock, flags);
	dev->in_ep->desc = NULL;
	dev->out_ep->desc = NULL;
	dev->interface = -1;
	spin_unlock_irqrestore(&dev->lock, flags);
}

static int printer_set_alt_interface(struct printer_dev *dev, unsigned number)
{
	int result = 0;

	printer_reset_interface(dev);
	result = printer_set_new_interface(dev);
	if (result)
		printer_reset_interface(dev);
	else
		dev->interface = number;

	if (!result)
		printk(KERN_ALERT "[printer device]""Using interface %x\n", number);
	return result;
}

static void printer_setup_softreset(struct printer_dev *dev)
{
	struct usb_request	*req;

	printk(KERN_ALERT "[printer device]""Received Printer Reset Request\n");

	if (usb_ep_disable(dev->in_ep))
		printk(KERN_ALERT "[printer device]""Failed to disable USB in_ep\n");
	if (usb_ep_disable(dev->out_ep))
		printk(KERN_ALERT "[printer device]""Failed to disable USB out_ep\n");

	if (dev->current_rx_req != NULL) {
		list_add(&dev->current_rx_req->list, &dev->rx_reqs);
		dev->current_rx_req = NULL;
	}
	dev->current_rx_bytes = 0;
	dev->current_rx_buf = NULL;
	dev->reset_printer = 1;

	while (likely(!(list_empty(&dev->rx_buffers)))) {
		req = container_of(dev->rx_buffers.next, struct usb_request, list);
		list_del_init(&req->list);
		list_add(&req->list, &dev->rx_reqs);
	}

	while (likely(!(list_empty(&dev->rx_reqs_active)))) {
		req = container_of(dev->rx_reqs_active.next, struct usb_request, list);
		list_del_init(&req->list);
		list_add(&req->list, &dev->rx_reqs);
	}

	while (likely(!(list_empty(&dev->tx_reqs_active)))) {
		req = container_of(dev->tx_reqs_active.next, struct usb_request, list);
		list_del_init(&req->list);
		list_add(&req->list, &dev->tx_reqs);
	}

	if (usb_ep_enable(dev->in_ep))
		printk(KERN_ALERT "[printer device]""Failed to enable USB in_ep\n");
	if (usb_ep_enable(dev->out_ep))
		printk(KERN_ALERT "[printer device]""Failed to enable USB out_ep\n");

	wake_up_interruptible(&dev->rx_wait);
	wake_up_interruptible(&dev->tx_wait);
	wake_up_interruptible(&dev->tx_flush_wait);
}

/***********************************************************************
 *													USB PROCESS
 ***********************************************************************/
static int printer_func_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct printer_dev *dev = container_of(f, struct printer_dev, function);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int value = -EOPNOTSUPP;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);

	printk(KERN_ALERT "[printer device]""ctrl req %02x.%02x v%04x i%04x l%d\n",
		ctrl->bRequestType, ctrl->bRequest, wValue, wIndex, wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_CLASS:
		switch (ctrl->bRequest) {
		case 0: /* Get the IEEE-1284 PNP String */
			/* Only one printer interface is supported. */
			if ((wIndex>>8) != dev->interface)
				break;

			value = (pnp_string[0]<<8) | pnp_string[1];
			memcpy(req->buf, pnp_string, value);
			printk(KERN_ALERT "[printer device]""1284 PNP String: %x %s\n", value, &pnp_string[2]);
			break;

		case 1: /* Get Port Status */
			/* Only one printer interface is supported. */
			if (wIndex != dev->interface)
				break;

			*(u8 *)req->buf = dev->printer_status;
			value = min(wLength, (u16) 1);
			break;

		case 2: /* Soft Reset */
			/* Only one printer interface is supported. */
			if (wIndex != dev->interface)
				break;

			printer_setup_softreset(dev);
			value = 0;
			break;

		default:
			goto unknown;
		}
		break;

	default:
unknown:
		printk(KERN_ALERT "[printer device]""unknown ctrl req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest, wValue, wIndex, wLength);
		break;
	}
	/* host either stalls (value < 0) or reports success */
	if (value >= 0) {
		req->length = value;
		req->zero = value < wLength;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			ERROR(dev, "%s:%d Error!\n", __func__, __LINE__);
			req->status = 0;
		}
	}
	return value;
}

static int __init printer_func_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct printer_dev *dev = container_of(f, struct printer_dev, function);
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *in_ep;
	struct usb_ep *out_ep = NULL;
	int id;
	int ret;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	intf_desc.bInterfaceNumber = id;

	/* all we really need is bulk IN/OUT */
	in_ep = usb_ep_autoconfig(cdev->gadget, &fs_ep_in_desc);
	if (!in_ep) {
autoconf_fail:
		dev_err(&cdev->gadget->dev, "can't autoconfigure on %s\n", cdev->gadget->name);
		return -ENODEV;
	}
	in_ep->driver_data = in_ep;	/* claim */

	out_ep = usb_ep_autoconfig(cdev->gadget, &fs_ep_out_desc);
	if (!out_ep)
		goto autoconf_fail;
	out_ep->driver_data = out_ep;	/* claim */

	/* assumes that all endpoints are dual-speed */
	hs_ep_in_desc.bEndpointAddress = fs_ep_in_desc.bEndpointAddress;
	hs_ep_out_desc.bEndpointAddress = fs_ep_out_desc.bEndpointAddress;

	ret = usb_assign_descriptors(f, fs_printer_function, hs_printer_function, NULL);
	if (ret)
		return ret;

	dev->in_ep = in_ep;
	dev->out_ep = out_ep;
	return 0;
}

static void printer_func_unbind(struct usb_configuration *c, struct usb_function *f)
{
	usb_free_all_descriptors(f);
}

static int printer_func_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct printer_dev *dev = container_of(f, struct printer_dev, function);
	int ret = -ENOTSUPP;
	if (!alt)
		ret = printer_set_alt_interface(dev, intf);
	return ret;
}

static void printer_func_disable(struct usb_function *f)
{
	struct printer_dev *dev = container_of(f, struct printer_dev, function);
	printk(KERN_ALERT "[printer device]""%s\n", __func__);
	printer_reset_interface(dev);
}

static void printer_cfg_unbind(struct usb_configuration *c)
{
	struct printer_dev *dev = &usb_printer_dev;
	struct usb_request *req;

	printk(KERN_ALERT "[printer device]""%s\n", __func__);
	device_destroy(usb_printer_class, usb_printer_devno);
	cdev_del(&dev->printer_cdev);
	WARN_ON(!list_empty(&dev->tx_reqs_active));
	WARN_ON(!list_empty(&dev->rx_reqs_active));

	while (!list_empty(&dev->tx_reqs)) {
		req = container_of(dev->tx_reqs.next, struct usb_request, list);
		list_del(&req->list);
		printer_req_free(dev->in_ep, req);
	}

	if (dev->current_rx_req != NULL)
		printer_req_free(dev->out_ep, dev->current_rx_req);

	while (!list_empty(&dev->rx_reqs)) {
		req = container_of(dev->rx_reqs.next, struct usb_request, list);
		list_del(&req->list);
		printer_req_free(dev->out_ep, req);
	}

	while (!list_empty(&dev->rx_buffers)) {
		req = container_of(dev->rx_buffers.next, struct usb_request, list);
		list_del(&req->list);
		printer_req_free(dev->out_ep, req);
	}
}

static int __init printer_bind_config(struct usb_configuration *c)
{
	struct usb_gadget *gadget = c->cdev->gadget;
	struct printer_dev *dev = &usb_printer_dev;
	int status = -ENOMEM;
	size_t len;
	u32 i;
	struct usb_request	*req;

	usb_ep_autoconfig_reset(gadget);

	dev->function.name = "printer";
	dev->function.bind = printer_func_bind;
	dev->function.setup = printer_func_setup;
	dev->function.unbind = printer_func_unbind;
	dev->function.set_alt = printer_func_set_alt;
	dev->function.disable = printer_func_disable;
	status = usb_add_function(c, &dev->function);
	if (status)
		return status;

	dev->pdev = device_create(usb_printer_class, NULL, usb_printer_devno, NULL, "wl_printer");
	if (IS_ERR(dev->pdev)) {
		ERROR(dev, "Failed to create device: wl_printer\n");
		goto fail;
	}

	cdev_init(&dev->printer_cdev, &printer_io_operations);
	dev->printer_cdev.owner = THIS_MODULE;
	status = cdev_add(&dev->printer_cdev, usb_printer_devno, 1);
	if (status) {
		ERROR(dev, "Failed to open char device\n");
		goto fail;
	}

	if (iPNPstring)
		strlcpy(&pnp_string[2], iPNPstring, (sizeof pnp_string)-2);
	len = strlen(pnp_string);
	pnp_string[0] = (len >> 8) & 0xFF;
	pnp_string[1] = len & 0xFF;

	usb_gadget_set_selfpowered(gadget);

	if (gadget->is_otg) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP;
		printer_cfg_driver.descriptors = otg_desc;
		printer_cfg_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	spin_lock_init(&dev->lock);
	mutex_init(&dev->lock_printer_io);
	INIT_LIST_HEAD(&dev->tx_reqs);
	INIT_LIST_HEAD(&dev->tx_reqs_active);
	INIT_LIST_HEAD(&dev->rx_reqs);
	INIT_LIST_HEAD(&dev->rx_reqs_active);
	INIT_LIST_HEAD(&dev->rx_buffers);
	init_waitqueue_head(&dev->rx_wait);
	init_waitqueue_head(&dev->tx_wait);
	init_waitqueue_head(&dev->tx_flush_wait);

	dev->interface = -1;
	dev->printer_cdev_open = 0;
	dev->printer_status = PRINTER_NOT_ERROR;
	dev->current_rx_req = NULL;
	dev->current_rx_bytes = 0;
	dev->current_rx_buf = NULL;

	for (i = 0; i < qlen; i++) {
		req = printer_req_alloc(dev->in_ep, USB_BUFSIZE, GFP_KERNEL);
		if (!req) {
			while (!list_empty(&dev->tx_reqs)) {
				req = container_of(dev->tx_reqs.next, struct usb_request, list);
				list_del(&req->list);
				printer_req_free(dev->in_ep, req);
			}
			return -ENOMEM;
		}
		list_add(&req->list, &dev->tx_reqs);
	}

	for (i = 0; i < qlen; i++) {
		req = printer_req_alloc(dev->out_ep, USB_BUFSIZE, GFP_KERNEL);
		if (!req) {
			while (!list_empty(&dev->rx_reqs)) {
				req = container_of(dev->rx_reqs.next, struct usb_request, list);
				list_del(&req->list);
				printer_req_free(dev->out_ep, req);
			}
			return -ENOMEM;
		}
		list_add(&req->list, &dev->rx_reqs);
	}

	/* finish hookup to lower layer ... */
	dev->gadget = gadget;
	printk(KERN_ALERT "[printer device]""printer configure end.\n");
	return 0;

fail:
	printer_cfg_unbind(c);
	return status;
}

static int printer_unbind(struct usb_composite_dev *cdev)
{
	// try to add list_del(&c->list) in __composite_unbind in composite.c
	printk(KERN_ALERT "[printer device]""printer unbind.\n");
	return 0;
}

static int __init printer_bind(struct usb_composite_dev *cdev)
{
	int ret;

	ret = usb_string_ids_tab(cdev, strings);
	if (ret < 0)
		return ret;
	device_desc.iManufacturer = strings[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings[USB_GADGET_SERIAL_IDX].id;

	printer_cfg_driver.unbind = printer_cfg_unbind;
	ret = usb_add_config(cdev, &printer_cfg_driver, printer_bind_config);
	if (ret)
		return ret;
	usb_composite_overwrite_options(cdev, &coverwrite);
	return ret;
}

static struct work_struct printer_resume_work;

static void printer_resume_late(struct work_struct *data)
{
	struct file *fp = filp_open("/sys/devices/soc.0/usbc0.5/otg_role", O_RDWR, 0644);
	char null_command[] = "null";
	char device_command[] = "usb_device";
	mm_segment_t fs;
	loff_t pos = 0;

	printk(KERN_ALERT "[printer device]""resume late.\n");
	if (IS_ERR(fp)){
		printk(KERN_ALERT "[printer device]""open file error\n");
		return;
	}
	mdelay(400);
	fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(fp, null_command, sizeof(null_command), &pos);
	mdelay(400);
	vfs_write(fp, device_command, sizeof(device_command), &pos);
	filp_close(fp, NULL);
	set_fs(fs);
}

void printer_disconnect(struct usb_composite_dev *cdev)
{
	printk(KERN_ALERT "[printer device]""disconnect.\n");
}

void printer_suspend(struct usb_composite_dev *cdev)
{
	printk(KERN_ALERT "[printer device]""suspend.\n");
	schedule_work(&printer_resume_work);
}

void printer_resume(struct usb_composite_dev *cdev)
{
	printk(KERN_ALERT "[printer device]""resume.\n");
	//schedule_work(&printer_resume_work);
}

static __refdata struct usb_composite_driver printer_driver = {
	.name  = "printer",
	.dev = &device_desc,
	.strings = dev_strings,
	.max_speed = USB_SPEED_HIGH,
	.bind = printer_bind,
	.unbind = printer_unbind,
	.disconnect = printer_disconnect,
	.suspend = printer_suspend,
	.resume = printer_resume,
};

static int __init printer_init(void)
{
	int status;
  printk(KERN_ALERT "[printer device]""Hello world.\n");

	usb_printer_class = class_create(THIS_MODULE, "usb_printer_dev");
	if (IS_ERR(usb_printer_class)) {
		status = PTR_ERR(usb_printer_class);
		printk(KERN_ALERT "[printer device]""unable to create usb_gadget class %d\n", status);
		return status;
	}

	status = alloc_chrdev_region(&usb_printer_devno, 0, 1, "USB printer device");
	if (status) {
		class_destroy(usb_printer_class);
		printk(KERN_ALERT "[printer device]""alloc_chrdev_region %d\n", status);
		return status;
	}

	status = usb_composite_probe(&printer_driver);
	if (status) {
		unregister_chrdev_region(usb_printer_devno, 1);
		class_destroy(usb_printer_class);
		printk(KERN_ALERT "[printer device]""usb_gadget_probe_driver %x\n", status);
	}

	INIT_WORK(&printer_resume_work, printer_resume_late);
	return status;
}

static void __exit printer_exit(void)
{
	printk(KERN_ALERT "[printer device]""Goodbye, cruel world.\n");
	usb_composite_unregister(&printer_driver);
	unregister_chrdev_region(usb_printer_devno, 1);
	class_destroy(usb_printer_class);
}

module_init(printer_init);
module_exit(printer_exit);

MODULE_AUTHOR("waterlemon");
MODULE_DESCRIPTION("printer device of waterlemon");
MODULE_LICENSE("GPL");
