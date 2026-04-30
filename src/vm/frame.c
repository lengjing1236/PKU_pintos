#include "frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

struct list frame_table;

void frame_table_init()
{
    list_init(&frame_table);
}

/* 分配一个frame，返回其对应的内核虚拟地址
 * 并且初始化frame table entry，将其添加到frame table中
 * 如果用户池不够，panic kernel
 */
void *
alloc_frame (void *upage)
{
    void *kpage = palloc_get_page(PAL_USER);
    if (kpage == NULL)
    {
        PANIC ("alloc frame: out of memory");
    }

    struct frame_table_entry *fte = malloc (sizeof *fte);
    if (fte == NULL)
    {
        palloc_free_page (kpage);
        PANIC ("malloc: out of memory");
    }

    fte->user_page = upage;
    fte->kernel_page = kpage;
    fte->accessed = false;
    fte->dirty = false;
    list_push_front (&frame_table, &fte->elem);

    return kpage;
}

/* 根据内核虚拟地址，在帧表中寻找对应的帧表项，
 * 从帧表中删除，并释放所有相关数据
 */
void free_frame(void *kpage)
{
    struct list_elem *e;
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
        struct frame_table_entry *fte = list_entry (e, struct frame_table_entry, elem);
        if (fte->kernel_page == kpage) {
            // 释放帧表项的相关内存
            list_remove (e);
            palloc_free_page (kpage);
            free (fte);
            return;
        }

    }
}