#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#define IRQ 9
#define DEVICE_NAME "stop"

void stop_start(void);
void stop_stop(void);
void irq_stop_register(void);

static struct fasync_struct *async;
static int stop_fasync(int fd, struct file *filp, int mode)
{
	printk(KERN_INFO "application fasync!\n");
	return fasync_helper(fd, filp, mode, &async);
}

static irqreturn_t irq_stop_handle()
{
	kill_fasync(&async, SIGIO, POLL_IN);
	return (IRQ_HANDLED);
}
int stop_open(struct inode *inode, struct file *filp)
{
	if(irq_stop_register() != 0)
	{
		printk(KERN_INFO "Request irq error!\n");
	}
	printk(KERN_ALERT "application open!\n");
	return 0;
}
ssize_t stop_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	printk(KERN_ALERT "application read!\n");
	return 0;
}
ssize_t stop_write(struct file *file, const char __user *buff, size_t count, loff_t *offp)
{
	printk(KERN_ALERT "application write!\n");
	return 0;
}
static int stop_release(struct inode *inode, struct file *file)
{
	disable_irq(IRQ);
	free_irq(IRQ, NULL);
	printk(KERN_ALERT "application release!\n");
	return stop_fasync(-1, file, 0);
}
static int stop_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_ALERT "application ioctl!\n");
	return 0;
}

static struct file_operations stop_ops = {
	.owner = THIS_MODULE,
	.open = stop_open,
	.release = stop_release,
	.ioctl = stop_ioctl,
	.read = stop_read,
	.write = stop_write,
	.fasync = stop_fasync,
};

static struct miscdevice stop_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &stop_ops,
};


int irq_stop_register(void)
{
	int err = request_irq(IRQ, irq_stop_handle, IRQF_DISABLED, "irq_stop", NULL);
	set_irq_type(IRQ, IRQF_TRIGGER_RISING);
	if(err)
	{
		disable_irq(IRQ);
		free_irq(IRQ, NULL);
		return -1;
	}
	return 0;
}

static int __init irq_stop_init(void)
{
	printk(KERN_INFO "module_init\n");
	int err = misc_register(&stop_misc);
	if(err < 0)
	{
		printk(KERN_ALERT "register miscdevice error code: %d\n", err);
		return err;
	}
	printk(KERN_INFO "stop device create!\n");
	return 0;
}


static void __exit irq_stop_exit(void)
{
	misc_deregister(&stop_misc);
	printk(KERN_INFO "module_exit\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("VOID0RED");
module_init(irq_stop_init);
module_exit(irq_stop_exit);

