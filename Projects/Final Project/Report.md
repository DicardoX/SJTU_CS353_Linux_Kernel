# *Dynamic Hijack on Clone System Call*

### Course Porject of CS353 Linux Kernel

*薛春宇 518021910698*

----------

## 1. 实验要求

​		本次实验旨在了解系统调用在内核中的调用方式，操作系统中页表的创建、遍历、修改和页表项各字段的含义，并掌握当自编写的驱动发生 *crash* 或 *panic* 时的调试方式。具体的实验要求如下：

1. 编写一个内核模块，要求替换系统调用表 (`sys_call_table`) 中的某一项系统调用，替换成自编写的系统调用处理函数，在新的系统调用函数中打印 "*hello, I have hacked this syscall*"，然后再调用执行原来的系统调用处理函数。以 `ilotl` 系统调用为例，它在系统调用表中的编号是 `__NR_ioctl`，则需要相应修改系统调用表中 `sys_call_table[__NR_ioctl]` 的指向，让其指向自编写的系统调用处理函数，并在打印相关信息后调用起原来指向的处理函数（实验环境 *ARM64* 和 *x86-64* 均可，内核版本要求 *Linux 5.0* 以上）。
2. 卸载模块时将系统调用表恢复原样。
3. 用 `clone` 系统调用来验证自编写的驱动，`clone` 的系统调用号是 `__NR_clone`。

---------



## 2. 实验环境

- 实验平台：华为云弹性云服务器（*2vCPUs | 4GiB | 2vCPUs | 4GiB | kc1.large.2 | Ubuntu 20.04 server 64bit with x86_64*）
- *Linux OS* 版本：*Ubuntu 20.04*
- 内核版本：*Linux-5.4.1*
- *GCC* 版本：9.3.0

--------



## 3. 实验内容

​		本部分中，将在 *3.1 - 3.3* 节分别介绍**系统调用表的获取**、**`clone` 系统调用处理函数的伪造**和**系统调用表的修改与恢复**三个模块的相关方法分析，以及源码层面上的实现方式。基于上述三个模块，我们将在 *3.4* 节给出本次实验的实现流程。本实验需要自编写一个内核模块，编译生成 `clone_hijack.ko` 并加载，实现系统调用表的修改和恢复。

----

#### 3.1 系统调用表的获取

###### 方法分析

​		首先，我们需要对系统调用 (*System Call*) 的相关概念进行学习和了解。系统调用 [*[1]*](#reference1) 指**运行在用户空间的程序向操作系统内核请求需要更高权限所运行的服务**。系统调用提供用户程序与操作系统之间的接口，当用户向内核请求某种内核态的服务（如文件 *I/O* 或进程间通信等）时，需要调用相应的系统调用，进而访问内核提供的系统资源。在收到系统调用请求后，操作系统一般通过中断的方式从用户态切换到内核态，执行相应的指令后再返回。

​		**系统调用在内核中的调用方式**为：在 *x86-64* 架构中，当处理器执行到与系统调用相关的汇编指令时，会触发软中断，经调度后进入内核态，切换至相应的系统调用处理函数进行执行。该**系统调用处理函数的地址被组织为系统调用表 (*System Call Table*) 的结构**，**页表定义在 `arch/x86/entry/syscall_64.c` 中**，表中的每一条页表项均包含一个指向该系统调用处理函数地址的指针，可以通过 *Linux* 多级页表结构进行分级查找。处理函数执行完成后，处理器返回结果，并再次通过软中断切换回用户态，继续执行上次被中断的任务。

```c
/* Line 21~28 in arch/x86/entry/syscall_64.c */
asmlinkage const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
	[0 ... __NR_syscall_max] = &sys_ni_syscall,
#include <asm/syscalls_64.h>
};
```

​		由上知，*Linux* 内核中的系统调用表大小由 `__NR_syscall_max` 定义，且由函数返回值可知，每个页表项都为 `sys_call_ptr_t` 类型的指针，该指针类型定义在 `arch/x86/include/asm/syscall.h` 中：

```c
/* Line 20-27 in arch/x86/include/asm/syscall.h */
#ifdef CONFIG_X86_64
typedef asmlinkage long (*sys_call_ptr_t)(const struct pt_regs *);
#else
// ...
#endif /* CONFIG_X86_64 */
extern const sys_call_ptr_t sys_call_table[];
```

​		 *x86_64* 架构中，`sys_call_ptr_t` 类型传入参数类型为 `pt_regs*`，该类型定义在 `arch/arc/include/asm/entry_arcv2.h` 中，描述了在执行该系统调用时，用户态下的 *CPU* 寄存器在内核态的栈中的保存情况。

​		**在获取 *Linux* 系统调用表的类型及定义方式后，我们还需要获取访问系统调用表的方法**。*x86_64* 下可通过向 `include/linux/kallsyms.h` 文件中的 `kallsyms_lookup_name()` 函数传递 `"sys_call_table"` 的方式，来获取系统调用表在内存中的起始地址。该函数接受一个字符串格式的内核函数名，返回相应的 `unsigned long` 类型的地址，在实现过程中需要转换为 `sys_call_pter_t*` 类型，以提供对系统调用表的直接访问。注意，必须在编译内核时启用 `CONFIG_KALLSYMS`，才可以使用 `kallsyms_lookup_name()` 函数 [*[2]*](#reference2)。

```c
/* Line 73-75 in include/linux/kallsyms.h */
#ifdef CONFIG_KALLSYMS
/* Lookup the address for a symbol. Returns 0 if not found. */
unsigned long kallsyms_lookup_name(const char *name);
```

###### 实现方式

​		基于上述分析，我们给出**从内存中获取系统调用表的实现方式**（位于自行实现的 `./clone_hijack.c` 文件中）：

1. 参考 *Linux* 中 `sys_call_ptr_t` 类型的实现，局部重定义一个自适应参数传入及返回值的函数指针类型 `sys_call_ptr_t`：

    ```c
    /* ptr type for address of system call */
    typedef void (*sys_call_ptr_t)(void);
    ```

2. 声明存储系统调用表起始地址的变量，并初始化为 `NULL`：

    ```c
    /* m_sys_call_table for the storage of start address in the obtained system call address */
    sys_call_ptr_t *m_sys_call_table = NULL;
    ```

3. 在模块初始化函数 `clone_hijack_init()` 中，使用上面提到的 `kallsyms_lookup_name()` 函数获取系统调用表的起始地址，并强制转换为 `sys_call_ptr_t*` 的格式：

    ```c
    /* Lookup the address of system call table */
    m_sys_call_table = (sys_call_ptr_t *)kallsyms_lookup_name("sys_call_table");
    ```

-------

#### 3.2 `clone` 系统调用处理函数的伪造

###### 方法分析及实现方式

​		*Linux* 中的进程创建主要由三种方式，分别是 `fork()`、`vfork()` 和 `clone()`。其中，`clone()` 是 *Linux* 为线程创建设计的，也可用于进程创建。不仅可以创建进程和线程，`clone()` 还可以在创建过程中提供个性化的选择，包括指定创建新的命名空间，有选择地继承父进程的内存，甚至可以将新创建的进程设置为父进程的兄弟进程 [*[3]*](#reference3) 。

​		在 *Linux* 内核中，`clone` 系统调用对应的处理函数是 `sys_clone()`：系统调用号是 `__NR_clone`，其中 `sys_clone()` 定义在 `include/linux/syscalls.h` 文件中：

```c
/* Line 841-853 in linux/include/syscalls.h */
/* arch/example/kernel/sys_example.c */
#ifdef CONFIG_CLONE_BACKWARDS
asmlinkage long sys_clone(unsigned long, unsigned long, int __user *, unsigned long,
	       int __user *);
#else
#ifdef CONFIG_CLONE_BACKWARDS3
asmlinkage long sys_clone(unsigned long, unsigned long, int, int __user *,
			  int __user *, unsigned long);
#else
asmlinkage long sys_clone(unsigned long, unsigned long, int __user *,
	       int __user *, unsigned long);
#endif
#endif
```

​		可知，`sys_clone` 系统调用的处理函数有三种参数实现方式，接下来在 `arch/Kconfig` 中寻找 `CONFIG_CLONE_BACKWARDS` 和 `CONFIG_CLONE_BACKWARDS3` 的宏定义，以**确定本实验需要使用的参数格式**。

```makefile
# Line 962-977 in arch/Kconfig
config CLONE_BACKWARDS
	bool
	help
	  Architecture has tls passed as the 4th argument of clone(2),
	  not the 5th one.
# ...
config CLONE_BACKWARDS3
	bool
	help
	  Architecture has tls passed as the 3rd argument of clone(2),
	  not the 5th one.
```

​		上述 `CLONE_BACKWARDS` 将线程局部存储 `tls` 作为 `sys_clone` 的第四个参数进行传递，而 `CLONE_BACKWARDS3` 则作为第三个参数。参考 [*[4]*](#reference4) 的分析可知， 我们应选择第三种实现方式，来设计伪造的 `clone` 系统调用处理函数。

​		**确定函数参数格式后，我们还需要定义相应的函数指针类型，并进行相应的实例化和指针赋值**。我们定义 `sys_clone_ptr_t` 类型的函数指针，并声明该指针的一个实例 `m_sys_clone_origin`，初始化为 `NULL`。随后，在模块初始化函数 `clone_hijack_init()` 中，将该指针指向原本的系统调用处理函数 `sys_clone()`，以便后续的访问和恢复。

```c
/* ptr type for the address of system call clone() */
typedef asmlinkage long (*sys_clone_ptr_t)(unsigned long, unsigned long, int __user *, int __user *, unsigned long);
// ...
/* m_sys_clone_origin for the storage of address in the function of original clone system call */
sys_clone_ptr_t m_sys_clone_origin = NULL;

/* Init function for this clone hijack module */
static int __init clone_hijack_init(void) 
{
	// ...
	/* Store the original address of clone system call */
	m_sys_clone_origin = (sys_clone_ptr_t)m_sys_call_table[__NR_clone];
  // ...
}
```

​		**完成函数指针的实例化后，我们开始实现伪造的系统调用处理函数**。该函数需要首先打印信息，其次是对真实处理函数 `sys_clone()` 的回调。因此，基于上面选择的函数参数格式，我们将该伪造的处理函数实现如下：

```c
/* Hijack function on clone system call */
asmlinkage long my_clone_syscall(unsigned long item_1, unsigned long item_2, int __user *ptr_1,
                                 int __user *ptr_2, unsigned long item_3) 
{
	printk(KERN_INFO "hello, I have hacked this syscall\n");
	return m_sys_clone_origin(item_1, item_2, ptr_1, ptr_2, item_3);
}
```

--------

#### 3.3 系统调用表的修改与恢复

###### 方法分析

​		在 *3.1* 和 *3.2* 节中，我们分别实现了系统调用表的获取，以及处理函数的伪造，在本节中，我们将**对系统调用表进行修改，以实现将`clone()` 系统调用指针重定向到伪造的处理函数上**。在退出模块时，我们同时需要将系统调用表恢复原状。

​		为了实现**对系统调用表的遍历，以获取系统调用表的 *entry***，我们需要通过定义在 `arch/x86/mm/pageattr.c` 中的内核地址查找函数 `lookup_address()` 来进行实现。该函数的作用是查找页表项的虚拟地址。 返回一个指向页表项的指针，和映射的级别。注意到，函数进一步调用了同样定义在 `pageattr.c` 中的 `lookup_address_in_pgd()` 函数 (*Line 566-605*)，因此**系统调用表的遍历查找过程**为：在当前五级页表结构下依次查询 `pgd`、`p4d`、`pud` 和 `pmd`，进而获取 `*pte`，该指针指向系统调用表的 `entry`。

```c
/* Line 615-618 in arch/x86/mm/pageattr.c */
pte_t *lookup_address(unsigned long address, unsigned int *level) {
    return lookup_address_in_pgd(pgd_offset_k(address), address, level);
}
```

​		**在成功获取系统调用表的 *entry* 之后，我们需要解除系统调用表所在内存页的写保护**，以便直接对系统调用表进行修改。具体来说，我们使用定义在 `arch/ia64/include/asm/pgtable.h` 中的宏 `pte_mkwrite` 来实现，该宏的作用是修改页描述符中的 `R/W` 字段，来实现该页读写权限的修改。

```c
/* Line 308 in arch/ia64/include/asm/pgtable.h */
#define pte_mkwrite(pte)	(__pte(pte_val(pte) | _PAGE_AR_RW))
```

​		同时，为了保证修改读写权限的原子性，我们使用定义在 `arch/x86/include/asm/pgtable.h` 中的宏 `set_pte_atomic` 来实现页读写字段修改的不可并行性，防止出现 `write panic`。

```c
/* Line 66 in arch/x86/include/asm/pgtable.h */
#define set_pte_atomic(ptep, pte)
```

​		**系统调用表的修改可以通过将伪造的函数经过强制转换后赋给系统调用表的 `clone` 目录项指针来完成**，具体实现将在下面的实现流程中给出。在完成修改后，我们使用定义在 `arch/x86/include/asm/pgtable.h` 中的宏 `pte_clear_flags` 来**恢复系统调用页的写保护**，该函数能够重置系统调用表的 `Flags`，进而清除之前设置的写权限。

```c
/* Line 306-311 in arch/x86/include/asm/pgtable.h */
static inline pte_t pte_clear_flags(pte_t pte, pteval_t clear) {
	pteval_t v = native_pte_val(pte);
	return native_make_pte(v & ~clear);
}
```

​		其中，定义在 `pgtable_types.h` 文件中的 `native_pte_val` 能够获取该 `pte` 页表的 *pte* 字段的值，并使用 `native_make_pte()` 宏来将和相应的 `pteval_t clear` 的值做 *&* 运算后的值制作为一个 `pte_t` 实例。当我们将 `clear` 参数设置为 `_PAGE_RW` 时，上述操作会重置当前 *pte* 的 `R/W` 字段。最后，在退出模块时，我们需要将系统调用表恢复原状，流程与上述工作大致相同。

```c
/* Line 437-445 in arch/x86/include/asm/pgtable.h */
static inline pte_t native_make_pte(pteval_t val) {
	return (pte_t) { .pte = val };
}

static inline pteval_t native_pte_val(pte_t pte) {
	return pte.pte;
}
```

###### 实现方式

​		基于上述分析，我们给出**系统调用表的修改及恢复的实现方式**（位于自行实现的 `./clone_hijack.c` 文件中）：

1. 完成 *3.1* 及 *3.2* 节中的系统调用表地址的获取和处理函数的伪造；

2. 声明 `pte_t` 类型的指针 `pte` 和级别参数 `level`， 利用 `look_address()` 函数实现对系统调用表的遍历，以获取系统调用表的 *entry*：

    ```c
    /* ptr for the entry of page table */
    pte_t *pte;
    /* level for the paging mechanism */
    unsigned int level;
    
    // ...
    
    /* Init function for this clone hijack module */
    static int __init clone_hijack_init(void) 
    {
    	// ...
    	/* Lookup the address for the entry of system calll table */
    	pte = lookup_address((unsigned long)m_sys_call_table, &level);
    	// ...
    }
    ```

3. 使用原子操作解除系统调用表的写保护，重定向 `clone()` 页表项的指针，以及恢复页表的写保护：

    ```c
    /* Init function for this clone hijack module */
    static int __init clone_hijack_init(void) 
    {
    	// ...
    	/* Change the permission of this pte to allow writing as an atomic op */
    	set_pte_atomic(pte, pte_mkwrite(*pte));
    
    	/* Overwrite the __NR_clone entry (originally point to clone system call) with 
    		address to our clone_hijack function */
    	m_sys_call_table[__NR_clone] = (sys_call_ptr_t)my_clone_syscall;
    
    	/* Rechange the permission of this pte to provide protection */
    	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
    
    	return 0;
    }
    ```

4. 在模块退出时，进行与 *step 3* 类似的操作，利用之前保存的真实处理函数来恢复系统调用表：

    ```c
    /* Exit function for this clone hijack module */
    static void __exit clone_hijack_exit(void) 
    {
    	/* Change the permission of this pte to allow writing as an atomic op */
    	set_pte_atomic(pte, pte_mkwrite(*pte));
    
    	/* Recover the __NR_clone entry (currently point to the hijack func) with 
    		address to the original clone system call func */
    	m_sys_call_table[__NR_clone] = (sys_call_ptr_t)m_sys_clone_origin;
    
    	/* Rechange the permission of this pte to provide protection */
    	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
    }
    ```

---------

#### 3.4 模块源码

​		本次项目实现模块的源码可见 [*Appendix A*](#appendixA)。

------



## 4.实验结果

#### 4.1 `Makefile` 编译及测试文件的准备

​		为了对本项目实现的模块进行编译，需要编写如下 `Makefile` 文件：

```makefile
obj-m := clone_hijack.o
KDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
```

​		本实验在 `x86-64` 架构下提供了 `test.o` 和 `bench.o` 两个测试用的可执行文件，需要将两个文件移动到指定目录下，并利用 `chmod` 指令修改其权限为可执行。

<img src="./cut/截屏2021-06-23 下午5.44.15.png" alt="avatar" style="zoom:40%;" />

注意，本实验的预期效果是：

- 输入 `dmesg`，系统日志的输出增加一次 `hack` 信息；
- 运行 `./test.o` 文件，系统日志的输出增加一次 `hack` 信息；
- 运行 `./bench.o` 文件：系统日志的输出增加六次 `hack` 信息；

--------

#### 4.2 结果分析

###### `dmesg`

​		调用 `dmesg -C` 指令清空系统日志输出信息，并使用 `dmesg` 进行验证。在使用 `insmod clone_hijack.ko` 指令完成内核模块的插入后，连续调用三次 `dmesg` 指令，可以看到随着每次指令的调用，系统日志中都会多输出一次 `hello, I have hacked this syscall` 的信息。

<img src="./cut/截屏2021-06-22 上午1.14.08.png" alt="avatar" style="zoom:40%;" />

###### `./test.o`

​		同样使用 `dmesg -C` 指令清空系统日志的输出，并使用 `dmesg` 进行验证，此时仅一条 `hack` 信息。随后使用 `./test` 运行该可执行文件，输出 *"hello world"* 信息，说明该文件正常执行。在系统日志输出内观察到三条 `hack` 信息，除去原先的信息外，一条是 `dmesg` 指令引起的输出，另一条则由 `test.o` 文件的执行引起。

<img src="./cut/截屏2021-06-22 上午1.16.49.png" alt="avatar" style="zoom:40%;" />

###### `./bench.o`

​		继续清空系统日志输出并验证，此时包含一条 `hack` 信息。使用 `./bench.o` 运行 另一个可执行文件，可以看到进行了一系列的 `clone` 操作，并输出了相关信息，说明该文件正常执行。随后调用 `dmesg` 查看系统输出日志，发现除原先的一条信息外，一条是 `dmesg` 指令引起的输出，另一条则由 `bench.o` 执行过程中对 `clone()` 函数的调用引起。

<img src="./cut/截屏2021-06-22 上午1.18.32.png" alt="avatar" style="zoom:40%;" />

​		结合上述实验效果分析可知，本次实验取得成功。

------



## 5. 实验心得

​		本次实验是 *Linux* 内核课程的课程设计，也是最后一次内核相关的实验，旨在掌握系统调用及页表相关的内容，包括系统调用处理函数的实现，页表遍历，以及系统调用表的修改和恢复。在系统调用处理函数的实现过程中，我掌握了 *Linux* 系统调用函数的底层处理机制，并分析了 `sys_clone()` 系统调用处理函数所需的参数格式及意义；同时，在实现页表遍历的过程中，我学习了 *Linux-5.4.1* 版本下内核采取的五级页表架构，并通过 `look_address` 等函数的分析深入理解了 *Linux* 环境下的寻址过程；最后，在实现系统调用表的修改和恢复的过程中，我知晓了 *Linux* 内存页的写保护方式，以及如何利用原子操作来更改特定页的读写权限。

​		在实验过程中，我首先根据实验指导书上的要求，将任务拆分成几个步骤依次执行，包括系统调用表的获取，系统调用处理函数的伪造，以及系统调用表的修改和恢复。在实现每个步骤时，我首先基于收集的资料确定实现相关功能的函数或宏，然后通过关键词匹配的方式在源码树中找到模块的关键函数和结构体，并依次进行调用函数的深入搜索、研究，最终细化到底层实现，力求完全掌握该结构体的功能及所调用函数的意义。由于目前网上针对 `sys_clone` 的教程相对较少，且内核版本也浮动较大，因此我必须在源码的基础上不断进行试错，不断修改报错的 *API* 或结构体类名。例如，在进行系统调用表的修改时，在一开始我并没有采取原子操作的方式对页读写权限进行修改，而是直接通过访问字段进行的简单修改，这直接导致我在源码编译室产生了 `write warning` 的报错，在经过一番尝试和查找资料后才得以解决。

​		本次实验中，我不仅更深一步地掌握了 *Linux* 源码阅读的方法，增强了自身阅读源码的能力，还提高了发现问题、解决问题的能力。感谢陈全老师的用心授课，和黄浩威助教认真负责的指导！



-----

## *References*

<a name="reference1"></a> *[1] Syscall in Wikipedia: https://en.wikipedia.org/wiki/System_call*

<a name="reference2"></a> *[2] http://blog.chinaunix.net/uid-24467128-id-3919206.html*

<a name="reference3"></a> *[3] https://blog.csdn.net/gogokongyin/article/details/51178257*

<a name="reference4"></a> *[4] https://blog.csdn.net/luoyhang003/article/details/47125787*

-----



<a name="appendixA"></a>

### *Appendix A*：clone_hijack模块的实现源码

```c
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

```

