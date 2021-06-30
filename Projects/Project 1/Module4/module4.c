 
//<module4.c>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>					// for basic filesystem
#include <linux/proc_fs.h>				// for the proc filesystem
#include <linux/seq_file.h>				// for sequence files
#include <linux/slab.h>					// for kzalloc, kfree
#include <linux/uaccess.h>    			// for copy_from_user

#define BUF_SIZE 10

// Message
static char *message = NULL;
// Directory and file
struct proc_dir_entry *helloworldDir, *helloworld;


// Show
static int hello_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "Successfully read content from proc file!\n");
	seq_printf(m, "The message is: %s\n", message);
	return 0;
}


// file_operations -> write
static ssize_t hello_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	// Create user buffer
	char *userBuffer = kzalloc((count + 1), GFP_KERNEL);
	if (!userBuffer) {
		return -ENOMEM;
	}
	// Copy the data in user space to kernel space
	if(copy_from_user(userBuffer, buffer, count)) {
		kfree(userBuffer);
		return EFAULT;
	}
	// Release the original string space
	kfree(message);
	// Redirect
	message = userBuffer;
	return count;
}

// seq_operations -> open
static int hello_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hello_proc_show, NULL);
}

// Specify file operations, we need to replace struct file_operations with struct proc_ops for kernel version 5.6 or later
static const struct proc_ops hello_proc_fops = {
	.proc_open = hello_proc_open,
	.proc_read = seq_read,
	.proc_write = hello_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

// module init
static int __init hello_init(void) {
	printk(KERN_INFO "Test for module4...\n");
	// Create proc directory
	helloworldDir =  proc_mkdir("helloworldDir", NULL);
	if (!helloworldDir) {
		return -ENOMEM;
	}
	// Create proc file
	helloworld = proc_create("helloworld", 0644, helloworldDir, &hello_proc_fops);
	if (!helloworld) {
		return -ENOMEM;
	}

	return 0;
}

// module exit
static void __exit hello_exit(void)
{
	// Remove file
	remove_proc_entry("helloworld", helloworldDir);
	// Remove dir
	remove_proc_entry("helloworldDir", NULL);

	printk(KERN_INFO "Bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module4");
MODULE_AUTHOR("Chunyu Xue");
