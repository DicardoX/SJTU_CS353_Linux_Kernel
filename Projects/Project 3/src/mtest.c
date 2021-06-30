//<mtest.c> 

// ------------- Basic -----------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>				// For ./proc operation
#include <linux/string.h>				// For using strcmp function
#include <asm/current.h>				// Get task_struct of current process
#include <linux/uaccess.h>
#include <linux/slab.h>

// --------- For read vma --------------
#include <linux/mm.h>
#include <linux/mm_types.h>

// --------- For find page -------------
#include <asm/pgtable.h>
#include <asm/page.h>



#define MAX_BUF_SIZE 128
static char proc_buf[MAX_BUF_SIZE];


/*Print all vma of the current process*/
static void mtest_list_vma(void) {
	// Pointer to the linked list in virtual memory 
	struct vm_area_struct *cur = current->mm->mmap;
	printk(KERN_INFO "Current process ID: %d", current->pid);

	while(cur) {
		// Permission
		char permission[5] = "----";

		// Read
		if(cur->vm_flags & VM_READ) {
			permission[0] = 'r';
		}
		// Write
		if(cur->vm_flags & VM_WRITE) {
			permission[1] = 'w';
		}
		// Execute
		if(cur->vm_flags & VM_EXEC) {
			permission[2] = 'x';
		}
		// Shared or not
		if(cur->vm_flags & VM_SHARED) {
			permission[3] = 's';
		} else {
			permission[3] = 'p';
		}

		printk(KERN_INFO "0x%lx 0x%lx %s\n", cur->vm_start, cur->vm_end, permission);
		cur = cur->vm_next;
	}
}


/* Find the corresponding page based on the vma */
static struct page* find_page_based_on_vma(struct vm_area_struct* vma, unsigned long addr) {
	// Memory descriptor of current process
	struct mm_struct *mm = vma->vm_mm;
	// Page global directory
	pgd_t *pgd = pgd_offset(mm, addr);
	// // Page 4 directory (new to 5 levels page table in Linux)
	// p4d_t *p4d = NULL;
	// Page upper directory
	pud_t *pud = NULL;
	// Page middle directory
	pmd_t *pmd = NULL;
	// Page table
	pte_t *pte = NULL;
	// Page
	struct page *page = NULL;

	// Check and assignment
	if(pgd_none(*pgd) || pgd_bad(*pgd)) { return NULL; }
	// // Assign to p4d
	// p4d = p4d_offset(pgd, addr);

	// if(p4d_none(*p4d) || p4d_bad(*p4d)) { return NULL; }
	// Assign to pud
	pud = pud_offset(pgd, addr);

	if(pud_none(*pud) || pud_bad(*pud)) { return NULL; }
	// Assign to pmd
	pmd = pmd_offset(pud, addr);

	if(pmd_none(*pmd) || pmd_bad(*pmd)) { return NULL; }
	// Assign to pte
	pte = pte_offset_map(pmd, addr);

	if(pte_none(*pte) || !pte_present(*pte)) { return NULL; }
	// Page
	page = pte_page(*pte);
	if(!page) { return NULL; }
	pte_unmap(pte);
	return page;
}


/*Find va->pa translation */
static void mtest_find_page(unsigned long addr) {
	// Page
	struct page *page = NULL;
	// Physical address
	unsigned long physical_addr;

	// Find vma
	struct vm_area_struct *vma = find_vma(current->mm, addr);
	if(!vma) {
		printk(KERN_ERR "Error occurred when finding vma...\n");
		return;
	}


	page = find_page_based_on_vma(vma, addr);
	if(!page) {
		printk(KERN_ERR "Error occurred when finding page based on vma...\n");
		return;
	}

	physical_addr = page_to_phys(page) | (addr & ~PAGE_MASK);
	printk(KERN_INFO "Virtual Address: 0x%lx  ->  Physical Address: 0x%lx\n", addr, physical_addr);
}


/*Write val to the specified address */
static void mtest_write_val(unsigned long addr, unsigned long val) {
	// Page
	struct page *page = NULL;
	// Virtual addr in kernel space
	unsigned long *kernel_addr;

	// Find vma
	struct vm_area_struct *vma = find_vma(current->mm, addr);
	if(!vma) {
		printk(KERN_ERR "Error occurred when finding vma...\n");
		return;
	}

	// If vm cannot be written
	if(!(vma->vm_flags & VM_WRITE)) {
		printk(KERN_ERR "Error occurred when writing to an unwritable page...");
		return;
	}

	page = find_page_based_on_vma(vma, addr);
	if(!page) {
		printk(KERN_ERR "Error occurred when finding page based on vma...\n");
		return;
	} else {
		kernel_addr = (unsigned long *)page_address(page);
		// Modify the value
		*kernel_addr = val;
		printk(KERN_INFO "Successfully write %ld into virtual address: 0x%lx\n", val, addr);
	}
}


// Implement the interface of proc file, different echo -> different operations
static ssize_t mtest_proc_write(struct file *file, const char __user * buffer, size_t count, loff_t * data) {
	// Address
	unsigned long int addr;
	// Value for write
	unsigned long int value;

	// Copy from user buffer
	int copy_return_msg = copy_from_user(proc_buf, buffer, count);
	if(copy_return_msg != 0) {
		printk(KERN_ERR "Error occurred when copying from user...\n");
        return -EFAULT;
	}

	if (strncmp(proc_buf, "listvma", 7) == 0) {
        printk(KERN_INFO "Operation: listvma\n");
        mtest_list_vma();
    }
    else if(strncmp(proc_buf, "findpage", 8) == 0) {
    	// Addr input
    	sscanf(proc_buf + 9, "%lx", &addr);

    	printk(KERN_INFO "Operation: findpage 0x%lx\n", addr);
    	mtest_find_page(addr);
    }
    else if(strncmp(proc_buf, "writeval", 8) == 0) {
    	// Addr and value input
    	sscanf(proc_buf + 9, "%lx %ld", &addr, &value);

        printk(KERN_INFO "Operation: writeval 0x%lx with %ld\n", addr, value);
        mtest_write_val(addr, value);
    }
    else {
        printk(KERN_ERR "Error occurred when getting input...\n");
    }


    return count;

}


// File operations interface
static struct file_operations proc_mtest_operations = {
    .owner = THIS_MODULE,
    .write = mtest_proc_write
};

static struct proc_dir_entry *mtest_proc_entry = NULL;


// Create proc file
static int __init mtest_init(void) { 
	mtest_proc_entry = proc_create("mtest", 0666, NULL, &proc_mtest_operations);
	if (!mtest_proc_entry) {
		// Error occurred when creating proc
		printk(KERN_ERR "Error occurred when creating /proc/mtest...\n");
        return -EINVAL;
	} else {
		printk(KERN_INFO "Successfully create /proc/mtest!\n");
		return 0;
	}
	return 0;
}


// Remove proc file
static void __exit mtest_exit(void) {
	proc_remove(mtest_proc_entry);
	printk(KERN_INFO "Removing mtest file from ./proc...\n");
}


module_init(mtest_init);
module_exit(mtest_exit);


