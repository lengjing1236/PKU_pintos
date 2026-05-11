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

void
SPT_init (struct thread *t)
{
    t->SPT = malloc (sizeof *t->SPT);
    if (t->SPT == NULL) {
        PANIC ("SPT malloc: out of memory");
    }
    hash_init (t->SPT, page_hash, page_hash_less, NULL);
}

struct SPT_entry *
SPTE_lookup (const void *address)
{
    struct thread *t = thread_current ();
    struct SPT_entry spte;
    struct hash_elem *e;

    spte.upage = address;
    e = hash_find (t->SPT, &spte.elem);
    return e != NULL ? hash_entry (e, struct SPT_entry, elem) : NULL;
}

void
SPT_destroy (struct thread *t)
{
    if (t->SPT != NULL) {
        hash_destroy (t->SPT, NULL);
        free (t->SPT);
    }

}