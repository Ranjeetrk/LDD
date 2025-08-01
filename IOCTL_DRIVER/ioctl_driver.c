#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>

#define WR_VALUE _IOW('w','r',int32_t *)
#define RD_VALUE _IOR('r','d',int32_t*)

int32_t value = 0; 
dev_t dev=0; 
static struct cdev ioctl_cdev;
static struct class *dev_class;

static struct task_struct *wait_thread;
DECLARE_WAIT_QUEUE_HEAD(wq);  //static wait queue initialization 
// wait_queue_head_t wq;
// init_waitqueue_head(&wq);  // dynamic method 
static int wait_queue_flag = 0; // 0: no event, 1: read, 2: exit
static int count = 0;
static DEFINE_SPINLOCK(flag_lock); // Protect wait_queue_flag
				   
static int wait_function(void *){

	int loopcount = 0; 
	pr_info("wait_function entry \n");
	while (!kthread_should_stop()) {
		pr_info("wait_function running...%d\n",++loopcount);
		wait_event_interruptible(wq, (wait_queue_flag !=0) );
		spin_lock(&flag_lock);
		if(wait_queue_flag ==1){
			pr_info("wakeup event came from ioctl read, count :%d\n", ++count);
			wait_queue_flag = 0;
			spin_unlock(&flag_lock);
		}
		else if(wait_queue_flag == 2){
			pr_info("wakeup event came from exit driver\n");
			wait_queue_flag = 0;
			spin_unlock(&flag_lock);
			break;
		}

	}
	pr_info("wait_function exiting\n");
	spin_lock(&flag_lock);
    	wait_thread = NULL; // Clear reference to prevent kthread_stop accessing freed task_struct
	spin_unlock(&flag_lock);
	do_exit(0);
	return 0;
}

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
			pr_info("Read function called, waking up thread\n");
			spin_lock(&flag_lock);
			wait_queue_flag = 1;
			spin_unlock(&flag_lock);
			wake_up(&wq);

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

	/* create kernel thread */
	wait_thread = kthread_create(wait_function, NULL, "waitThread");
	if(wait_thread){
		pr_info("wait_function thread created \n");
		wake_up_process(wait_thread);
	}
	else
		pr_info("wait_function thread creation failed \n");

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
	if (wait_thread) {
		wait_queue_flag = 2;
		wake_up(&wq);
		kthread_stop(wait_thread);
		pr_info("wait_thread stopped\n");
	}
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

