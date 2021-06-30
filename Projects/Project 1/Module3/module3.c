//<module3.c>
//Read-only proc file
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>

static char *str = "Successfully read content from proc file!";

// Obtain info
static int hello_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "%s\n", str);
	seq_printf(m, "Current kernel time is: %ld\n", jiffies);

	return 0;
}

// Definition of file operations
static int hello_proc_open(struct inode *inode, struct file *file) { 
	return single_open(file, hello_proc_show, NULL);
}

// Specify file operations, we need to replace struct file_operations with struct proc_ops for kernel version 5.6 or later
static const struct proc_ops hello_proc_fops = { 
	.proc_open = hello_proc_open,
  	.proc_read = seq_read,
  	.proc_lseek = seq_lseek,
  	.proc_release = single_release,
};

// static const struct file_operations hello_proc_fops = { 
	//.owner = THIS_MODULE,
	//.open = hello_proc_open,
	//.release = single_release,
	//.llseek = seq_lseek,
	//.read = seq_read,
	// .write = NULL,
//};

static int __init hello_proc_init(void) { 
	printk(KERN_INFO "Test for module3...\n");
	// Create proc file
	proc_create("helloworld", 0444, NULL, &hello_proc_fops);

	return 0;
}

static void __exit hello_proc_exit(void) {
	// Remove proc file
	remove_proc_entry("helloworld", NULL);
	printk(KERN_INFO "Bye.\n");
}

module_init(hello_proc_init);
module_exit(hello_proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module3");
MODULE_AUTHOR("Chunyu Xue");
