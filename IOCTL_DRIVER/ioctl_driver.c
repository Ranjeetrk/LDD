#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define WR_VALUE _IOW('w','r',int32_t *)
#define RD_VALUE _IOR('r','d',int32_t*)

int32_t value = 0; 
dev_t dev=0; 
static struct cdev ioctl_cdev;
static struct class *dev_class;

static long my_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
		case WR_VALUE: 
			if(copy_from_user(&value, (int __user *)arg, sizeof(value)))
			{
				pr_err("data write error \n");
				return -EFAULT;
			}
			pr_info("data written value: %d \n", value);
			break;
		case RD_VALUE:
			if(copy_to_user((int __user*)arg, &value, sizeof(value)))
			{
				pr_err("data read failed \n");
				return -EFAULT;
			}
			break;
	}
	return 0;
}
static int ioctl_open(struct inode *inode,struct file *file)
{

	pr_info("device file opened \n");
	return 0;
}
static int ioctl_release(struct inode *inode, struct file *file)
{

	pr_info("Device file closed !!\n");
	return 0;
}

static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.open 		= ioctl_open,
	.release	= ioctl_release,
	.unlocked_ioctl = my_ioctl,
};

static int __init ioctl_driver_init(void)
{
	if(alloc_chrdev_region(&dev,0,1,"ioctl_drv"))
	{
		pr_err("alloc char device falied \n");
		return -ENOMEM;
	}
	cdev_init(&ioctl_cdev, &fops);
	if((cdev_add(&ioctl_cdev,dev,1)) < 0){
		pr_err("Cannot add the device to the system\n");
		goto r_clean;
	}
	if(IS_ERR(dev_class = class_create(THIS_MODULE, "ioctl_class"))){
		pr_err("class create failed \n");
		goto r_del;
	}
	/*Creating device*/
	if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"ioctl_device"))){
		pr_err("Cannot create the Device 1\n");
		goto r_device;
	}
	pr_info("IOCTL Driver Initialized\n");
	return 0;
r_device: 
	class_destroy(dev_class);
r_del:
	cdev_del(&ioctl_cdev);

r_clean:
	unregister_chrdev_region(dev,1);

	return -ENOMEM;
}

static void __exit ioctl_driver_exit(void)
{
	device_destroy(dev_class, dev);  // to ensure /dev/ioctl_device is removed
	class_destroy(dev_class);
	cdev_del(&ioctl_cdev);
	unregister_chrdev_region(dev,1);
	pr_info("IOCTL Driver Exited\n");
}

module_init(ioctl_driver_init);
module_exit(ioctl_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RANJEET <ranjeetrk1998@gmail.com>");
MODULE_DESCRIPTION("A simple IOCTL driver example");

