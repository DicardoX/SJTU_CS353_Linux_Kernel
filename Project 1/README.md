# Project1: 内核编程基础



## 1. Linux驱动程序中最重要的三个内核数据结构

[Ref: file_operations结构体解析](http://blog.chinaunix.net/uid-25100840-id-304208.html)

- **Overline**

  - `inode`：文件，储存文件元信息的索引节点
  - `file`：文件描述符
  - `file_operations`:说明设备驱动的接口

  在Linux中，**inode结构用于表示文件**，而**file结构则表示打开的文件的描述**，因为**对于单个文件而言可能会有许多个表示打开的文件的描述符**（[文件描述符](https://blog.csdn.net/guzizai2007/article/details/84652616)是一个简单的整数，用以表明每一个被进程所打开的文件或socket），因为就可能会存在有**多个file结构同时指向单个inode结构**。

  在系统内部，I/O设备的存取操作通过特定的入口进行，而**这组特定的入口由驱动程序来提供**，通常**这组设备驱动的接口是由file_operations结构体向系统说明的**。

  

- **inode** 文件索引节点详解（**储存文件元信息的索引节点**）：[Ref：理解Linux文件系统之inode](https://blog.csdn.net/haiross/article/details/39157885)：

  - 理解inode，要从文件储存说起。文件储存在硬盘上，硬盘的最小存储单位叫做"扇区"（Sector）。每个扇区储存512字节（相当于0.5KB）。操作系统读取硬盘的时候，不会一个个扇区地读取，这样效率太低，而是一次性连续读取多个扇区，即一次性读取一个**"块" (block)**。这种**由多个扇区组成的"块"，是文件存取的最小单位**。"块"的大小，最常见的是4KB，即连续八个 sector组成一个 block。
  - **文件数据都储存在"块"中**，那么很显然，我们**还必须找到一个地方储存文件的元信息，比如文件的创建者、文件的创建日期、文件的大小等等**。这种**储存文件元信息的区域就叫做inode，中文译名为"索引节点"**。
  - 每一个文件都有对应的inode，里面包含了与该文件有关的一些信息（**除文件名以外的所有文件信息**）。inode的**内容**包括：
    - 文件的字节数
    - 文件拥有者的User ID
    - 文件的Group ID
    - 文件的读、写、执行权限
    - 文件的时间戳（共有三个）：
      - `ctime`指向inode上一次变动的时间
      - `mtime`指向文件内容上一次变动的时间
      - `atime`指向文件上一次被打开的时间
    - 链接数：即有多少个文件名指向这个inode
    - 文件数据块 (block) 的位置
  - **stat指令查看文件的inode信息**：`stat file.txt`
  - **inode的大小**：
    - **inode也会消耗硬盘空间**，所以硬盘格式化的时候，**操作系统自动将硬盘分成两个区域**。一个是**数据区，存放文件数据**；另一个是**inode区（inode table），存放inode所包含的信息**。
    - 每个inode节点的大小，一般是128字节或256字节。**inode节点的总数，在格式化时就给定，一般是每1KB或每2KB就设置一个inode**。假定在一块1GB的硬盘中，每个inode节点的大小为128字节，每1KB就设置一个inode，那么inode table的大小就会达到128MB，占整块硬盘的12.8%。
  - **df指令查看每个硬盘分区的inode总数及已经使用的数量**：`df -i`
  - **查看每个inode节点的大小**：`sudo dumpe2fs -h /dev/hda | grep "Inode size"`
  - <font color=blue>注意</font>：由于每个文件都必须有一个inode，因此**有可能发生inode已经用光，但是硬盘还未存满的情况**。这时，就**无法在硬盘上创建新文件**。
  - **inode号码**：
    - 每个inode都有一个号码，**操作系统用inode号码来识别不同的文件**。
    - **Unix/Linux系统内部不使用文件名，而使用inode号码来识别文件**。对于系统来说，**文件名只是inode号码便于识别的别称或者绰号**。
    - 表面上，用户通过文件名，打开文件。实际上，系统内部这个过程分成三步：
      - 首先，系统找到这个文件名对应的**inode号码**
      - 其次，通过inode号码，获取**inode信息**
      - 最后，根据inode信息，找到**文件数据所在的block**，读出数据
    - **ls -i指令查看文件名对应的inode号码**：`ls -i file.txt`
  - **目录文件 (directory)** ：也是一种文件
    - 由一系列**目录项 (dirent) **组成的列表，每个目录项由两个部分组成：
      - 所包含文件的**文件名**
      - 该文件名对应的**inode号码**
    - `ls filefolder`：只列出目录文件中的所有文件名
    - `ls -i filefolder`：列出目录文件中的所有文件名及对应的inode号码
    - `ls -l filefolder`：查看文件的详细信息
  - **硬链接 (hard link)**：
    - 一般情况下，文件名和inode号码是一一对应关系，每个inode号码对应一个文件名。**但是，Unix/Linux系统允许，多个文件名指向同一个inode号码，共享同一块数据块**。
    - 硬链接的定义：
      - **可以用不同的文件名访问同样的内容**
      - **对文件内容进行修改，会影响到所有文件名**
      - **删除一个文件名，不影响另一个文件名的访问**
    - **硬链接的创建：`ln 源文件 目标文件`**
      - 运行该指令后，源文件与目标文件的inode号码相同，都指向同一个inode，共享同一块数据块。（inode信息中有一项叫做链接数，记录指向该inode的文件名总数）
    - **只能对已存在的文件进行创建**
    - **不能对目录进行创建**
    - **不可以交叉文件系统进行创建**
    - 删除一个文件名，就会使得inode节点中的链接数减1。**当链接数减到0，表明没有文件名指向这个inode，系统就会回收这个inode号码，以及其所对应block区域**。
    - **目录的两个默认目录项**：**创建目录时，默认会生成两个目录项："."和".."**：
      - "."目录项的inode号码就是**当前目录的inode号码，等同于当前目录的硬链接**
      - ".."目录项的的inode号码就是**当前目录的父目录的inode号码，等同于父目录的硬链接**
  - **软链接 (soft link)** / **符号链接 (symbolic link)**：
    - 软链接的定义：
      - **文件A和文件B的inode号码虽然不一样**，但是**文件A的内容是文件B的路径**。读取文件A时，系统会自动将访问者导向文件B。因此，无论打开哪一个文件，最终读取的都是文件B。
    - **文件A依赖于文件B而存在**，如果删除了文件B，打开文件A就会报错：`No such file or directory`
    - **文件A指向文件B的文件名，而不是文件B的inode号码，文件B的inode的链接数不会因此发生变化**。(<font color=blue>软链接与硬链接最大的不同</font>)
    - **软链接有自己的文件属性及权限等（文件A）**
    - **可以对不存在的文件或目录创建**
    - **可以交叉文件系统进行创建**
    - 可以对文件或**目录**进行创建
    - **软链接的创建：`ln -s 源文件或目录 目标文件或目录`** 
  - **inode的特殊作用**：
    - **移动文件或重命名文件，只是改变文件名，不影响inode号码**
    - 通常来说，**系统无法从inode号码得知文件名**
      - 使得**软件更新**变得简单，**可以在不关闭软件的情况下进行更新，不需要重启**。因为**系统通过inode号码，识别运行中的文件，不通过文件名**。更新的时候，**新版文件以同样的文件名，生成一个新的inode，不会影响到运行中的文件**。等到**下一次运行这个软件的时候，文件名就自动指向新版文件，旧版文件的inode则被回收**。
    - 有时，文件名包含特殊字符，无法正常删除。这时，直接删除inode节点，就能起到删除文件的作用



- **flie_operations** 主要成员函数详解：
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

[Ref: [linux 内核] 自建内核模块，并以proc文件形式提供外部访问接口，可通过读取此文件读内核数据](https://blog.csdn.net/ykun089/article/details/106376044/)

- `proc_create` OR `create_proc_entry` OR `create_proc_read_entry`

- `static inline struct proc_dir_entry *proc_create(const char *name, mode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops)`
  - `name`：创建的文件名
  - `mode`：访问权限
  - `parent`：父文件夹的`proc_dir_entry`对象
  - `proc_fops`：该文件的操作函数

- 在使用`proc_create()`函数时，编译报错接收的指针对象应该是**const struct proc_ops ***而非**const struct file_operations ***，解决方案是[将文件操作结构的定义更新](https://stackoverflow.com/questions/64931555/how-to-fix-error-passing-argument-4-of-proc-create-from-incompatible-pointer)，在内核5.6或更新版本中适用

- **seq_file机制**
  - 普通的文件没有一定的组织结构，文件可以从任意位置开始读写。有一种文件与普通文件不同，它**包含一系列的记录，而这些记录按照相同的格式来组织**，这种文件称为**顺序文件 (sequential file)** 。**seq_file是专门处理顺序文件的接口**
  - 使用seq_file需要包含**头文件linux/seq_file.h**
  - **seq_file的常用操作接口**：
    - **seq_open**：`int seq_open(struct file*, const struct seq_operations*)`：打开文件
    - **seq_read**：`ssize_t seq_read(struct file*, char __user*, size_t, loff_t*)`：读文件
    - **seq_lseek**：`loff_t seq_lseek(struct file*, loff_t, int)`：定位
    - **seq_release**：`int seq_release(struct inode*, struct file*)`：释放
    - **seq_escape**：`int seq_escape(struct seq_file*, const char*, const char*)`：写缓冲，忽略某些字符
    - **seq_putc**：`int seq_putc(struct seq_file *m, char c)`：把一个字符输出到seq_file文件
    - **seq_puts**：`int seq_puts(struct seq_file *m, char *s)`：把一个字符串输出到seq_file文件
  - <font color=blue>注意</font>：**seq_read、seq_lseek、seq_release等函数可以直接赋给file_operations文件操作结构的成员**。seq_file结构通常保存在file结构的private_data中。







































