#ifndef PAGE_H
#define PAGE_H

#include "hash.h"
#include "frame.h"
#include "filesys/off_t.h"
#include "threads/thread.h"

enum location
{
    IN_MEMORY,
    IN_SWAP,
    IN_FILE
};

struct SPT_entry {
    void *upage;                        // 用户虚拟地址
    struct frame_table_entry *frame;    // 用户page实际对应的帧表项
    enum location page_location;        // 数据实际存储位置 
    off_t ofs;                          // 数据开始处的偏移量

    struct hash_elem elem;              // 哈希表元素
};

void SPT_init (struct thread *t);
struct SPT_entry *SPTE_lookup(const void *address);
void SPT_destroy (struct thread *t);

#endif // page.h