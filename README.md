# CS353_Linux_Kernel

------------

 - Linux内核笔记：
    - Linux Kernel 编程基础

 - Project 0: 编译、安装Linux Kernel

 - Project 1: Linux内核模块编程基础
   - 模块一：加载和卸载模块，该模块可输出部分信息

   - 模块二：该模块可以接收三种类型的参数 (整型、字符串、数组)

   - 模块三：该模块创建一个proc文件, 读取该文件并返回部分信息（创建文件要使用proc_create）

   - 模块四：该模块在/proc下创建文件夹，并创建一个可读可写的proc文件（**write**实现需要使用kzalloc、kfree进行内存分配，创建文件夹要使用proc_mkdir）
