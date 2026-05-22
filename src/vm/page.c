#include "page.h"
#include "threads/malloc.h"

/* Returns a hash value for SPT_entry. */
static unsigned
page_hash (const struct hash_elem *e, void *aux UNUSED) 
{
    const struct SPT_entry *spte = hash_entry (e, struct SPT_entry, elem);
    return hash_bytes (&spte->upage, sizeof spte->upage);
}

/* Returns true if upage a precedes upage b. */
static bool
page_hash_less (const struct hash_elem *a_, 
                 const struct hash_elem *b_, void *aux UNUSED)
{
    const struct SPT_entry *a = hash_entry (a_, struct SPT_entry, elem);
    const struct SPT_entry *b = hash_entry (b_, struct SPT_entry, elem);
    return a->upage < b->upage;
}

/* 释放spte指向的帧的内存 */
static void 
hash_free (struct hash_elem *e, void *aux UNUSED)
{
    struct SPT_entry *spte = hash_entry (e, struct SPT_entry, elem);

    // 不需要释放spte指向的page，已经在pagedir_destroy里面释放了
    // if (spte->page_location == IN_MEMORY)
    // {
    //     free_frame_by_fte (spte->frame);
    // }

    // 释放spte本身
    free (spte);
}

/** 为线程t的补充页表分配内存并初始化 */
void
SPT_init (struct thread *t)
{
    t->SPT = malloc (sizeof *t->SPT);
    if (t->SPT == NULL) {
        PANIC ("SPT malloc: out of memory");
    }
    hash_init (t->SPT, page_hash, page_hash_less, NULL);
}

/** 销毁并释放线程t的补充页表 */
void
SPT_destroy (struct thread *t)
{
    if (t->SPT != NULL) {
        hash_destroy (t->SPT, hash_free);
        free (t->SPT);
    }

}

/** SPTE的创建和初始化 */
struct SPT_entry *SPTE_create (void *upage, enum location location, bool writable)
{
    struct SPT_entry *spte = malloc (sizeof *spte);
    if (spte == NULL) {
        PANIC ("malloc error : create spte fail");
    }
    spte->upage = upage;
    spte->page_location = location;
    spte->writable = writable;

    return spte;
}

/** 从当前线程的补充页表中查找address对应的SPTE */
struct SPT_entry *
SPTE_lookup (const void *address)
{
    struct thread *cur = thread_current ();
    struct SPT_entry spte;
    struct hash_elem *e;

    spte.upage = address;
    e = hash_find (cur->SPT, &spte.elem);
    return e != NULL ? hash_entry (e, struct SPT_entry, elem) : NULL;
}

/** 将新的SPTE插入当前线程的补充页表 */
bool SPTE_insert (struct hash_elem *h_elem)
{
    struct thread *cur = thread_current ();

    return hash_insert (cur->SPT, h_elem) == NULL ? true : false;
}

/** 从当前线程的补充页表中删除对应的SPTE */
bool SPTE_delete (struct hash_elem *h_elem)
{
    struct thread *cur = thread_current ();

    return hash_delete (cur->SPT, h_elem) != NULL ? true : false;
}