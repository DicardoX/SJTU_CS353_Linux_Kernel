# CS353_Linux_Kernel

------------

###### Notes

 - Linux内核笔记：
    - Lecture1：Linux Kernel内核概述
    - Lecture2：进程管理
    - Lecture3：进程管理(2)_进程调度
    - Lecture4：中断管理
    - Lecture5：中断管理(2)_内核同步
    - Lecture6：中断管理(3)_对称多处理SMP
    - Lecture7：内存管理
    - Lecture8：虚拟文件系统
    - Lecture9：电源管理



-------------------

###### Projects

 - **Project 0**：**编译、安装Linux Kernel**

 - **Project 1**：**Linux内核模块编程基础**
   
   - 模块一：加载和卸载模块，该模块可输出部分信息
   - 模块二：该模块可以接收三种类型的参数 (整型、字符串、数组)
   - 模块三：该模块创建一个proc文件, 读取该文件并返回部分信息（创建文件要使用proc_create）
   - 模块四：该模块在/proc下创建文件夹，并创建一个可读可写的proc文件（**write**实现需要使用kzalloc、kfree进行内存分配，创建文件夹要使用proc_mkdir）

- **Project2**：**进程管理**

    - **为 `task_struct` 结构添加数据成员 `int ctx`，每当进程被调用一次，`ctx++`把 *ctx* 输出到 `/proc/<PID>/ctx` 下，通过 `cat /proc/<PID>/ctx` 可以查看当前指定进程中 *ctx* 的值**

- **Project3**：**内存管理**

    - 写一个模块 *mtest*，当模块被加载时，创建一个 *proc* 文件 `/proc/mtest`，该文件接收三种类型的参数，具体如下：

        - `listvma`：打印当前进程的所有虚拟内存地址，打印格式为 `start_addr end_addr permission`
        - `findpage addr`：把当前进程的虚拟地址转化为物理地址并打印，如果不存在这样的翻译，则输出 `Error occurred when finding page based on vma...`
        - `writeval addr val`：向当前地址的指定虚拟地址中写入一个值。

- **Project4**：**文件系统**

    - 以 *Linux* 内核中的 `/fs/romfs` 作为文件系统源码修改的基础，实现以下功能，其中 `romfs.ko` 接受三种参数：`hided_file_name`, `encrypted_file_name` 和 `exec_file_name`：

        - `hided_file_name=xxx`：隐藏名字为 `xxx` 的文件或路径；
        - `encrypted_file_name=xxx`：加密名字为 `xxx` 的文件中的内容；
        - `exec_file_name=xxx`：修改名字为 `xxx` 的文件的权限为可执行。

        ​		上述功能通过生成并挂载一个格式为 `romfs` 的镜像文件 `test.img` 来检查，镜像文件可通过 `genromfs` 来生成。

- **Final Project**：**系统调用劫持**

    - 本次实验旨在了解系统调用在内核中的调用方式，操作系统中页表的创建、遍历、修改和页表项各字段的含义，并掌握当自编写的驱动发生 *crash* 或 *panic* 时的调试方式。具体的实验要求如下：

        1. 编写一个内核模块，要求替换系统调用表 (`sys_call_table`) 中的某一项系统调用，替换成自编写的系统调用处理函数，在新的系统调用函数中打印 "*hello, I have hacked this syscall*"，然后再调用执行原来的系统调用处理函数。以 `ilotl` 系统调用为例，它在系统调用表中的编号是 `__NR_ioctl`，则需要相应修改系统调用表中 `sys_call_table[__NR_ioctl]` 的指向，让其指向自编写的系统调用处理函数，并在打印相关信息后调用起原来指向的处理函数（实验环境 *ARM64* 和 *x86-64* 均可，内核版本要求 *Linux 5.0* 以上）。
        2. 卸载模块时将系统调用表恢复原样。
        3. 用 `clone` 系统调用来验证自编写的驱动，`clone` 的系统调用号是 `__NR_clone`。
