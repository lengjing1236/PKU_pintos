#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

struct exec_aux
{
    char *cmdline;                      // 命令行整体
    struct child_status *child_stat;    // 共享子进程的状态
};

struct child_status 
{
    tid_t child_tid;                // 子进程的tid

    bool load_success;              // load() result
    struct semaphore load_sema;     // 用来让父进程等待子进程load完毕

    int exit_status;                // 子进程退出状态
    bool is_exit;                   // 子进程是否退出
    struct semaphore exit_sema;     // 用来让父进程等待子进程exit

    struct lock ref_lock;           // 保护引用计数的互斥访问
    int ref_cnt;                    // 引用计数，=0时释放资源

    struct list_elem elem;          // 父进程维护的子进程状态链表的元素
};

/* per-process fd entry */
struct file_desc {
  int fd;
  struct file *file;
  struct list_elem elem;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /**< userprog/process.h */
