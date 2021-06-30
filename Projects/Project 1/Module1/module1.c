// <module1.c>
// Test for installing and removing of module
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

// Enterance
static int __init hello_init(void) {
	printk(KERN_INFO "Test for Module1...\n");
	return 0;
}

// Exitance
static void __exit hello_exit(void) {
	printk(KERN_INFO "Bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module1");
MODULE_AUTHOR("Chunyu Xue");
