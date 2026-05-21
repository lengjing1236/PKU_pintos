#ifndef FRAME_H
#define FRAME_H

#include <lib/stdio.h>
#include <list.h>
#include "page.h"

struct frame_table_entry {
    void *user_page;        // 指向当前占用该frame的user page
    void *kernel_page;      // 底层palloc实际分配的物理frame的起始内核虚拟地址，palloc使用
    
    // 用来实现驱逐策略
    bool accessed;          // 访问位
    bool dirty;             // 脏位

    struct list_elem elem;  // 哈希表元素
};

void frame_table_init (void);
void *alloc_frame_for_upage (void *upage, struct SPT_entry *spte);
void free_frame (void *kpage);

#endif // frame.h