/* clone_hijack.c */

/* Basic */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

/* kallsyms module records the address of vars and funcs except for stack vars */
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chunyu Xue");
MODULE_DESCRIPTION("a LKM to implement hijack on system call clone()");

/* ptr type for address of system call */
typedef void (*sys_call_ptr_t)(void);
/* ptr type for the address of system call clone() */
typedef asmlinkage long (*sys_clone_ptr_t)(unsigned long, unsigned long, int __user *, int __user *, unsigned long);

/* m_sys_call_table for the storage of start address in the obtained system call address */
sys_call_ptr_t *m_sys_call_table = NULL;
/* m_sys_clone_origin for the storage of address in the function of original clone system call */
sys_clone_ptr_t m_sys_clone_origin = NULL;

/* ptr for the entry of page table */
pte_t *pte;
/* level for the paging mechanism */
unsigned int level;


/* Hijack function on clone system call */
asmlinkage long my_clone_syscall(unsigned long item_1, unsigned long item_2, int __user *ptr_1,
                                 int __user *ptr_2, unsigned long item_3) 
{
	printk(KERN_INFO "hello, I have hacked this syscall\n");
	return m_sys_clone_origin(item_1, item_2, ptr_1, ptr_2, item_3);
}


/* Init function for this clone hijack module */
static int __init clone_hijack_init(void) 
{
	printk(KERN_INFO "Installation of clone_hijack module is processing...\n");

	/* Lookup the address of system call table */
	m_sys_call_table = (sys_call_ptr_t *)kallsyms_lookup_name("sys_call_table");

	/* Store the original address of clone system call */
	m_sys_clone_origin = (sys_clone_ptr_t)m_sys_call_table[__NR_clone];

	/* Lookup the address for the entry of system calll table */
	pte = lookup_address((unsigned long)m_sys_call_table, &level);

	/* Change the permission of this pte to allow writing as an atomic op */
	set_pte_atomic(pte, pte_mkwrite(*pte));

	/* Overwrite the __NR_clone entry (originally point to clone system call) with 
		address to our clone_hijack function */
	m_sys_call_table[__NR_clone] = (sys_call_ptr_t)my_clone_syscall;

	/* Rechange the permission of this pte to provide protection */
	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));

	return 0;
}


/* Exit function for this clone hijack module */
static void __exit clone_hijack_exit(void) 
{
	printk(KERN_INFO "Uninstallation of clone_hijack module is processing...\n");
	
	/* Change the permission of this pte to allow writing as an atomic op */
	set_pte_atomic(pte, pte_mkwrite(*pte));

	/* Recover the __NR_clone entry (currently point to the hijack func) with 
		address to the original clone system call func */
	m_sys_call_table[__NR_clone] = (sys_call_ptr_t)m_sys_clone_origin;

	/* Rechange the permission of this pte to provide protection */
	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
}


module_init(clone_hijack_init);
module_exit(clone_hijack_exit);
