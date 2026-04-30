#ifndef FRAME_H
#define FRAME_H

#include <lib/stdio.h>
#include <list.h>

struct frame_table_entry {
    void *user_page;        // 指向当前占用该frame的user page
    void *kernel_page;      // 底层palloc实际分配的物理frame的起始内核虚拟地址
    
    // 用来实现驱逐策略
    bool accessed;          // 访问位
    bool dirty;             // 脏位

    struct list_elem elem;  // 哈希表元素
};

void frame_table_init ();
void *alloc_frame (void *upage);
void free_frame (void *kpage);

#endif // frame.h