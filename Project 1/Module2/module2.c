// <module2.c>
// Support for int & str & array parameter
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/string.h>

// Definition for parameters
static int int_var = -9999;
static char *str_var = "Default";
static int int_array[10];
// Number of elements in array
int arrNum;

// Set interface
module_param(int_var, int, 0644);
MODULE_PARM_DESC(int_var, "An integer variable");			// Description of parameter
module_param(str_var, charp, 0644);
MODULE_PARM_DESC(str_var, "A string variable");
module_param_array(int_array, int, &arrNum, 0644);
MODULE_PARM_DESC(int_array, "An integer array");



// Enterance
static int __init hello_init(void) {
	int i;
	
	if (int_var == -9999 && strcmp(str_var, "Default") == 0 && arrNum == 0)
	{
		printk(KERN_INFO "No parameters input, exit.\n");
		return 0;
	}
	
	printk(KERN_INFO "The parameters are:\n");
	
	if (int_var != -9999) {
		printk(KERN_INFO "Int: %d\n", int_var);
	}
	
	if (strcmp(str_var, "Default") != 0) {
		printk(KERN_INFO "Str: %s\n", str_var);
	}

	if (arrNum != 0) {
		for(i = 0; i < arrNum; i++)
		{
			printk(KERN_INFO "Int_array[%d]: %d\n", i, int_array[i]);
		}
	}

	return 0;
}

// Exitance
static void __exit hello_exit(void) {
	printk(KERN_INFO "Bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module2");
MODULE_AUTHOR("Chunyu Xue");
