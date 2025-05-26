/*
 * Character device drivers lab
 *
 * All tasks
e*/

#include "linux/printk.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/errno.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");
MODULE_INFO(intree, "Y");

#define MY_MAJOR 42
#define MY_MINOR 0
#define NUM_MINORS 1
#define MODULE_NAME "so2_cdev"
#define MESSAGE "hello\n"
#define IOCTL_MESSAGE "ioctl called\n"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

struct so2_device_data {
	struct cdev cdev; /* TODO 2: add cdev member */
	atomic_t access; /* TODO 3: add atomic_t access variable to keep track if file is opened */
	char buffer[BUFSIZ]; /* TODO 4: add buffer with BUFSIZ elements */
	/* TODO 7: extra members for home */
};

struct so2_device_data devs[NUM_MINORS]; //instance of so2_device_data

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;

	/* TODO 2: print message when the device file is open. */
	pr_info("Device file opened!");

	/* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
	data = container_of(inode->i_cdev, struct so2_device_data, cdev);

	file->private_data = data;

#ifndef EXTRA
	/* TODO 3: return immediately if access is != 0, use atomic_cmpxchg */
	if (atomic_cmpxchg(&data->access, 0, 1)) {
		pr_err("Device is busy, access denied");
		return -EBUSY;
	}
#endif

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10 * HZ);

	return 0;
}

static int so2_cdev_release(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;
	pr_info("Device file closed!"); /* TODO 2: print message when the device file is closed. */

#ifndef EXTRA
	data = (struct so2_device_data *)file->private_data;

	/* TODO 3: reset access variable to 0, use atomic_set */
	atomic_set(&data->access, 0);

#endif
	return 0;
}

static ssize_t so2_cdev_read(struct file *file, char __user *user_buffer,
			     size_t size, loff_t *offset)
{
	size_t to_read = 0;
	int len;
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;

#ifdef EXTRA
	/* TODO 7: extra tasks for home */
#endif

	/* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
	len = strlen(data->buffer);

	if (*offset >= len)
		return 0;

	to_read = min(size, (size_t)(len - *offset));

	if (copy_to_user(user_buffer, data->buffer, to_read)) {
		return -EFAULT;
	}
	*offset += to_read;

	return to_read;
}

static ssize_t so2_cdev_write(struct file *file, const char __user *user_buffer,
			      size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;

	/* TODO 5: copy user_buffer to data->buffer, use copy_from_user */
	size_t to_write;

	//Limit the size to prevent buffer overflow
	to_write = min(size, (size_t)(BUFSIZ - 1));

	//Copy data from user space
	if (copy_from_user(data->buffer, user_buffer, to_write)) {
		return -EFAULT;
	}

	// Null terminate to make sure buffer is valid C string
	data->buffer[to_write] = '\0';

	/* TODO 7: extra tasks for home */

	return size;
}

static long so2_cdev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;
	int ret = 0;
	// int remains;

	switch (cmd) {
	case MY_IOCTL_PRINT:
		/* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
		pr_info(IOCTL_MESSAGE);
		break;
	case MY_IOCTL_SET_BUFFER:
		if (copy_from_user(data->buffer, (char __user *)arg,
				   BUFFER_SIZE)) {
			return -EFAULT;
		}
		data->buffer[BUFFER_SIZE - 1] = '\0';
		break;
	case MY_IOCTL_GET_BUFFER:
		if (copy_to_user((char __user *)arg, data->buffer,
				 BUFFER_SIZE)) {
			return -EFAULT;
		}
		break;
	case MY_IOCTL_DOWN:
		pr_info("IOCTL DOWN called\n");
		break;
	case MY_IOCTL_UP:
		pr_info("IOCTL UP called\n");
		break;

	/* TODO 7: extra tasks, for home */
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
	.open = so2_cdev_open, /* TODO 2: add open and release functions */
	.release = so2_cdev_release,
	.read = so2_cdev_read, /* TODO 4: add read function */
	.write = so2_cdev_write, /* TODO 5: add write function */
	.unlocked_ioctl = so2_cdev_ioctl, /* TODO 6: add ioctl function */
};

static int so2_cdev_init(void)
{
	int err;
	int i;

	/* TODO 1: register char device region for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS,
				     MODULE_NAME);
	// pr_info("register success ");
	if (err != 0) {
		pr_info("register_chrdev_region fail with %d\n", err);
		return err;
	}

	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		/* TODO 7: extra tasks, for home */
#else
		/*TODO 4: initialize buffer with MESSAGE string */
		strcpy(devs[i].buffer, MESSAGE);
		/* TODO 3: set access variable to 0, use atomic_set */
		atomic_set(&devs[i].access, 0);
#endif
		/* TODO 7: extra tasks for home */

		/* TODO 2: init and add cdev to kernel core */
		cdev_init(&devs[i].cdev, &so2_fops);
		devs[i].cdev.owner = THIS_MODULE;
		err = cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, MY_MINOR + i), 1);
		if (err) {
			pr_info("Error adding cdev: %d\n", err);
			return err;
		}
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++) {
		/* TODO 2: delete cdev from kernel core */
		cdev_del(&devs[i].cdev);
	}

	/* TODO 1: unregister char device region, for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
