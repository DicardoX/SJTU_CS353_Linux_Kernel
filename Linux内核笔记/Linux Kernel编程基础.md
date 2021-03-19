# Linux Kernel编程基础

**Outline**

- Linux Kernel编程基础
- /proc文件系统
- /proc文件系统模块编程

--------

## 1. Linux Kernel编程基础

### 1.1 模块`Module`

- 什么是模块？`An object file that contains code to extend the running kernel`/ `Modules are pieces of code that can be loaded and unloaded into the kernel upon demand.`

  使用场景：大多数类Unix及Windows系统

  <img src="./pic/截屏2021-03-15 下午4.16.21.png" alt="avatar " style="zoom:60%;" />

- **注意**：
  
  - 模块中只可以使用由Kernel和其他内核开放的API，**没有库**
  - 模块**运行在`ring 0 `权限，存在潜在安全漏洞**
  - 基本**不可以使用C++编写模块**
  
- 使用模块的**优点**：
  
  - 允许内核在运行过程中**动态插入/删除代码**
  - **节省内存开销** （若不支持动态加载，则不使用的内核功能会占据内存）
  
- 使用模块的**缺点**：
  - **碎片化** （产生内存碎片，降低内存性能，可通过内存管理部分规避）
  - **不兼容性** 容易导致系统崩溃

- **模块加载流程**：
  - `insmod`指令
    	 - `init function()` 初始化函数
        	 - `blk_init_queue()`函数：初始化块设备
           	 - `add_disk()`函数： add_disk()是块设备注册的内核接口，是块设备驱动的最后一步，也最关键
                	 - `request_queue`：请求队列
                	 - `request()`请求
  - `rmmod`指令：
    - `cleanup function()` 清除函数
    - `del_gendisk()`函数：与`add_disk()`函数对应
    - `blk_cleanup_queue()`函数：清除块设备

<img src="./pic/截屏2021-03-15 下午4.28.45.png" alt="avatar " style="zoom:60%;" />



- 模块编译 —— `Makefile`
  - `-C` ：将目录改为所提供的目录，即内核源码目录（当前正在运行linux kernel的源代码的目录），可以找到内核的顶层`Makefile`
  - `M`：将目录转移回模块源码所在目录，并进行编译
  - `obj-m:=` 编译目标

<img src="./pic/截屏2021-03-15 下午5.00.53.png" alt="avatar" style="zoom:50%;" />

​		另一种方式：

<img src="./pic/截屏2021-03-15 下午4.53.46.png" alt="avtar" style="zoom:50%;" />

- **模块相关指令**：
  - `insmod hello.ko` ：插入模块
  - `rmmod hello.ko`：删除模块
  - `lsmod` 列出现有模块（`lsmod | grep module` 列出特定名称的已加载模块）
  - `modinfo hello.ko` ： 列出特定模块的信息
  - `modprobe`：插入模块，并自动处理相关存在依赖关系的模块



- 向模块**提供参数**：
  - `static int test` ：声明变量
  - `module_param(name, type, perm)`：定义模块参数，`name`参数名，`type`类型，`perm`参数的访问权限
  - `EXPORT_SYMBOL(hello_foo)`：将变量权限改为其他内核模块也可访问

<img src="./pic/截屏2021-03-15 下午5.04.48.png" alt="avatar" style="zoom:50%;" />

-------



## 2. `/proc`文件系统

### 2.1 特征

- **伪文件系统** `pseudo file system`
- 实时变化，**存在于虚拟内存中**（不存放于任何存储介质），**文件夹大小是0**（不占据硬盘空间），**修改时间是上次启动的时间**
- 追踪、记录系统状态以及进程状态
- **每次Linux重启时，都会创建新的`/proc`文件系统**
- 内容必须由具有**权限**的用户读取，某些部分只可由拥有者和根用户读取
- 从`/proc`特定目录中抓取数据的指令：`top`、`ps`、`lspci`、`dmesg`（log信息）
- `/proc/kcore`:
  - 系统中**物理内存的一个别名，不占硬盘空间**
  - **存在实际大小**（和大多数/proc文件不同），为物理内存的大小 + 4KB，单位为字节
  - 内容通常由debugger来检查和使用（如GDB），只有根用户Root才能有查看权限

----------



### 2.2 通过`/proc`查看系统信息

- `buddyinfo`：包含内核`Buddy`系统（buddy system）中各个大小空闲区域的数量
- `cmdline`：内核command line
- `cpuinfo`**处理器信息**（Human readable）
- `devices`：当前正在运行的内核中**设备驱动的列表**（块设备 + 字符设备）
- `dma`：显示目前哪些DMA通道正在使用
- `fb`：**帧缓存** (frame buffer)设备
- `filesystems`：内核中**配置/支持的文件系统**
- `interrupts`：X86体系结构中每个**IRQ (Interrupt Request)支持的中断数量**
- `iomem`：显示当前**各种设备系统的IO内存资源的分布**
- `ioports`：记录**已注册的用作输入输出的端口列表**
- `kmsg`：包含**由内核生成的各种信息**，这些信息可以由特定的程序和指令读取，如klogd
- `loadavg`：显示**系统的平均负载 (average load)**
- `locks`：显示当前**已被内核锁定的文件**
- `mdstat`：包含系统中**磁盘和RAID的实时信息**
- `meminfo`：
  - /proc中最常用到的文件之一
  - 包含大量当前系统中**内存的各种信息及使用情况**
- `misc`：包含**系统中注册的各种混杂设备驱动**
- `modules`：显示了系统中**已经加载的所有模块的列表**
- `mounts`：显示了系统中**所有已挂载的设备的列表**
- `partitions`：给出系统中**可用的各个分区的详细信息**
- `pci`：列出系统中所有PCI设备
- `slabinfo`：给出**slab系统中内存使用的各种信息**（Slab是Linux使用的一种内存分配机制）
- `stat`：追踪系统重启后各种统计信息
- `swap`：记录系统中**交换空间 (swap space)及其利用率**
- `uptime`：系统**上次启动后的运行时间**，重启归零
- `version`：当前使用的**Linux内核及gcc的版本**，以及系统中安装的Linux的版本
- **以数字命名的目录**（**进程目录**）：
  - 包含**\proc文件系统做快照的瞬间正在运行的进程的相关信息**
  - **内容类别相同，但值不同**，**对应进程的各种参数以及运行状态**
  - 各用户**仅对其自身启动的进程具有完全的访问权限**
  - 内容：
    - `cmdline`：包含**调用该进程的完整命令行指令信息**（指令 + 参数）
    - `cwd`：链接到**当前工作目录的符号链接**
    - `environ`：包含该进程特定的所有**环境变量**
    - `exe`：**可执行文件的符号链接**
    - `maps`：该进程的**部分地址空间**
    - `fd`：包含**由该进程打开的所有文件描述符 (file descriptors)**
    - `root`：**指向该进程根文件系统的符号链接**
    - `status`：**该进程的相关状态信息**
- `self`：链接向**当前正在运行的进程**
- `bus`：包含系统中**可用的各种总线的相关信息**
- `driver`：内核**正在使用的驱动**
- `fs`：特定文件系统，文件句柄，inode，dentry以及quata信息
- `ide`：IDE设备信息（`Integrated Drive Electronics`，集成驱动器电子装置，指把控制器与盘体集成在一起的硬盘驱动器）
- `irq`：用来**设置IRQ和CPU的亲和性/分配**
- `net`：**网络参数及其统计信息**
- `scsi`：给出`scsi`设备的相关信息

-----------



### 2.3 通过`/proc`调整内核参数

- `/proc/sys`：修改该目录中的内容，用户可以实时修改特定的内核参数，重启后所有修改都消失。

  举例：`/proc/sys/net/ipv4/ip_forward`，默认值为0，可通过**实时**将其值修改为1，来允许IP转发

  `echo 1 >  proc/sys/net/ipv4/ip_forward`

--------



## 2.4 `/proc`文件系统的优缺点

- **优点**
  - Linux**内核信息的统一获取接口**
  - **调整和收集状态信息**
  - **容易使用**和编程
- **缺点**
  - 具有**部分开销**，必须使用`fs`调用（可使用`sysctl()`接口部分缓解）
  - 用户可能导致系统不稳定

---------



## 3. `/proc`文件系统模块编程

- **必须包含正确的头文件才可以使用`procfs`函数**

  `#include <linux/proc_fs.h>`

- `/proc`文件系统中的**文件创建**：

  - `struct proc_dir_entry* create_proc_entry(const char* name, mode_t mode, struct proc_dir_entry* parent)`
  - 在目录`parent`下创建一个名为`name`、模式为`mode`的文件
  - 若是在`procfs`根目录下创建，则将`parent`设置为`null`
  - 创建成功，则返回一个指向新创建的`struct proc_dir_entry`的指针
  - **文件创建命令**：`foo_file = create_proc_entry(“foo”, 0644, example_dir)`

- `/proc`文件系统中的**目录创建**：

  - `struct proc_dir_entry* proc_mkdir(const char* name, struct proc_dir_entry* parent)`
  - 在`procfs`目录的名为`parent`的目录下创建一个新的目录`name`

- `/proc`文件系统中的**符号链接（类似于快捷方式）创建**：

  - `struct proc_dir_entry* proc_symlink(const char* name, struct proc_dir_entry* parent, const char* dest)`
  - 在`procfs`目录的名为`parent`的目录下创建一个新的名为`name`，指向`dest`的符号链接

- `/proc`文件系统中的**移除文件**：

  - `void remove_proc_entry(const char* name, struct proc_dir_entry* parent)`
  - 在`procfs`目录的名为`parent`的目录下移除一个文件`name`
  - 在调用`remove_proc_entry`之前，必须释放掉`struct proc_dir_entry`结构中的数据项









