# Project1: 内核编程基础



## 1. Linux驱动程序中最重要的三个内核数据结构

[Ref: file_operations结构体解析](http://blog.chinaunix.net/uid-25100840-id-304208.html)

- **Overline**

  - `inode`：文件
  - `file`：文件描述符
  - `file_operations`:说明设备驱动的接口

  在Linux中，**inode结构用于表示文件**，而**file结构则表示打开的文件的描述**，因为**对于单个文件而言可能会有许多个表示打开的文件的描述符**（[文件描述符](https://blog.csdn.net/guzizai2007/article/details/84652616)是一个简单的整数，用以表明每一个被进程所打开的文件或socket），因为就可能会存在有**多个file结构同时指向单个inode结构**。

  在系统内部，I/O设备的存取操作通过特定的入口进行，而**这组特定的入口由驱动程序来提供**，通常**这组设备驱动的接口是由file_operations结构体向系统说明的**。

- `flie_operations`主要成员函数详解：
  -  `struct module *owner`（**`.owner`**）：并非一个操作，是一个指向拥有这个结构的模块的指针，**防止模块还在使用的时候被卸载**，一般被简单初始化为`.owner = THIS_MODULE`
  - `loff_t (*llseek) (struct file *filp, loff_t p, int orig)`（**`.llseek`**）：
    - `filp`：指针参数，为进行读取信息的目标文件结构体指针
    - `p`：参数，为文件定位的目标偏移量
    - `orig`：参数，为对文件定位的起始地址（文件开头`SEEK_SET` (0) 等）
    - 用作**改变文件中的当前读/写位置**，返回值为新的位置（正值）
  - `ssize_t (*read) (struct file *filp, char __user *buffer, size_t size, loff_t *p)`（**`.read`**）：
    - `filp`：指针参数，为进行读取信息的目标文件结构体指针
    - `buffer`：指针参数，为对应放置信息的缓冲区（用户空间内存地址）
    - `size`：参数，要读取的信息长度
    - `p`：参数，读的位置相对于文件开头的偏移（读取信息后一般会移动，移动值为信息长度）
    - 这个函数用来**从设备中获取数据**。一个**非负返回值代表了成功读取的字节数**（`signed size`类型，常常是目标平台本地的整数类型）
  - `ssize_t (*write) (struct file *filp, const char __user *buffer, size_t count, loff_t *ppos)`（**`.write`**）：
    - `filp`：指针参数，为进行写入信息的目标文件结构体指针
    - `buffer`：指针参数，为要写入文件的信息缓冲区
    - `count`：要写入信息的长度
    - `ppos`：为当前的偏移位置（通常用来判断文件是否越界）
    - 这个函数用来**发送数据给设备**。一个**非负返回值代表成功写入的字节数**
  - `int (*open)(struct inode *inode , struct file *filp)`（**`.open`**）：
    - `inode`：**文件节点，这个节点只有一个**（**无论用户打开了多少个文件，都只对应一个inode结构**）
    - `filp`：指针参数，为打开的目标文件结构体指针。**只要打开一个文件，就对应着一个`file`结构体**，通常用来**追踪文件在运行时的状态信息**
    - 常常是对设备文件进行的第一个操作，也并不要求驱动一定要声明一个对应的方法，如果为`null`，则设备打开一直成功，但驱动并不会得到通知
  - `void (*release)(struct inode *inode, struct file *filp);`（**`.release`**）：
    - 当最后一个打开设备的用户进程执行`close()`系统调用的时候，内核将调用驱动程序`release()`函数
    - 主要任务是**清理未结束的输入输出操作，释放资源**，用户自定义排他位置的复位等
    - 对设备文件进行的最后一个操作，可以为`null`

--------------



## 2. `/proc`文件系统创建文件

[Ref: [linux 内核] 自建内核模块，并以proc文件形式提供外部访问接口，可通过读取此文件读内核数据](https://blog.csdn.net/ykun089/article/details/106376044/

- `proc_create` OR `create_proc_entry`

- `static inline struct proc_dir_entry *proc_create(const char *name, mode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops)`
  - `name`：创建的文件名
  - `mode`：访问权限
  - `parent`：父文件夹的`proc_dir_entry`对象
  - `proc_fops`：该文件的操作函数