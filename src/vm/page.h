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
    
    /* 如果在文件中，需要以下字段 */
    struct file *file;                  // 数据实际存储的文件指针
    off_t ofs;                          // 数据开始处的偏移量
    size_t read_bytes;                  // 要读取的字节数 
    size_t zero_bytes;                  // 要清零的字节数

    bool writable;                      // 可写/只读
    struct hash_elem elem;              // 哈希表元素
};

/** 生命周期管理. */
void SPT_init (struct thread *t);
void SPT_destroy (struct thread *t);

/** SPTE的创建和初始化 */
struct SPT_entry *SPTE_create (void *upage, enum location location, bool writable);

/** 搜索，插入，删除 */
struct SPT_entry *SPTE_lookup (const void *address);
bool SPTE_insert (struct hash_elem *);
bool SPTE_delete (struct hash_elem *);

#endif // page.h