//
// Created by yzh on 2019/6/24.
//

#include "string.h"
#include "frame.h"
#include "swap.h"
#include "page.h"
#include <devices/block.h>
#include <threads/vaddr.h>
#include <stdio.h>
#include <userprog/syscall.h>
#include "../threads/thread.h"
#include "../userprog/pagedir.h"
#include "../threads/malloc.h"
#include "../lib/stddef.h"
#include "../lib/kernel/hash.h"

static struct hash frame_table;

static struct lock frame_lock;

static struct list frame_clock_list;

struct frame_table_entry* cur_fte;

void frame_init(){
    hash_init(&frame_table, frame_hash, frame_hash_less, NULL);
    list_init(&frame_clock_list);
    lock_init(&frame_lock);
    cur_fte = NULL;
}

void* frame_find_entry(void* frame){
    struct frame_table_entry tmp;
    tmp.frame = frame;
    struct hash_elem*  he = hash_find(&frame_table, &tmp.hash_elem);
    return he == NULL? NULL : hash_entry(he, struct frame_table_entry, hash_elem);
}

void* frame_apply(enum palloc_flags flag, void* va) {
    lock_acquire(&frame_lock);

    ASSERT (pg_ofs (va) == 0);
    ASSERT (is_user_vaddr (va));

    void* frame = palloc_get_page(PAL_USER | flag);

    if (frame == NULL){
        frame = frame_replace(va);
        if (flag & PAL_ZERO)
            memset (frame, 0, PGSIZE);
        if (flag & PAL_ASSERT)
            PANIC ("frame_apply_fail");
    }

    if (frame == NULL){
        lock_release(&frame_lock);
        return NULL;
    }

    ASSERT(pg_ofs(frame) == 0);

    struct frame_table_entry* fte = (struct frame_table_entry*)malloc(sizeof (struct frame_table_entry));
    fte->frame = frame;
    fte->va = va;
    fte->t = thread_current();
    fte->pinned = true;
    hash_insert(&frame_table, &fte->hash_elem);

    lock_release(&frame_lock);
    return frame;
}

void* frame_replace(void* va){
    ASSERT(cur_fte != NULL);
    while(pagedir_is_accessed(cur_fte->t->pagedir, cur_fte->va)){
        pagedir_set_accessed(cur_fte->t->pagedir, cur_fte->va, false);
        frame_clock_next();
        ASSERT( cur_fte != NULL );
    }

    struct frame_table_entry* fte = cur_fte;
    void* tmp_frame = fte->frame;
    swap_address sa = (swap_address)-1;
    ASSERT(page_find_entry(cur_fte->t->page_table, cur_fte->va) != NULL);
    struct page_table_entry *pte = page_find_entry(cur_fte->t->page_table, cur_fte->va);
    if (pte == NULL || pte->fa == NULL || ((struct mmap_handler *)(pte->fa))->is_static){
        sa = swap_store(cur_fte->frame);
        if (sa == -1)
            return NULL;
        ASSERT(page_eviction(cur_fte->t, cur_fte->va, sa, true));
    }
    else{
        mmap_write(pte->fa, cur_fte->va, tmp_frame);
        ASSERT(page_eviction(cur_fte->t, cur_fte->va, sa, false));
    }

    list_remove(&fte->list_elem);
    if (list_empty(&frame_clock_list))
        cur_fte = NULL;
    else
        frame_clock_next();
    hash_delete(&frame_table, &fte->hash_elem);
    free(fte);
    return tmp_frame;
}

void frame_free(void* frame){
    lock_acquire(&frame_lock);

    struct frame_table_entry* fte = frame_find_entry(frame);
    if (fte == NULL)
        PANIC("what the fuck? where is the frame to be free?");
    if (!fte->pinned){
        if (cur_fte == fte){
            if (list_empty(&frame_clock_list))
                cur_fte = NULL;
            else
                frame_clock_next();
        }
        list_remove(&fte->list_elem);
    }
    hash_delete(&frame_table, &fte->hash_elem);
    free(fte);
    palloc_free_page(frame);

    lock_release(&frame_lock);
}

bool frame_reset(void* frame){
    lock_acquire(&frame_lock);

    struct frame_table_entry* fte = frame_find_entry(frame);
    if (fte == NULL){
        lock_release(&frame_lock);
        return false;
    }
    if (fte->pinned == false){
        lock_release(&frame_lock);
        return true;
    }

    fte->pinned = false;
    list_push_back(&frame_clock_list, &fte->list_elem);
    if (list_size(&frame_clock_list) == 1)
        cur_fte = fte;

    lock_release(&frame_lock);
    return true;
}

void frame_clock_next(){
    ASSERT(cur_fte != NULL);
    if (list_size(&frame_clock_list) == 1)
        return;;
    if (&cur_fte->list_elem == list_back(&frame_clock_list))
        cur_fte = list_entry(list_head(&frame_clock_list),
                                   struct frame_table_entry, list_elem);

    cur_fte = list_entry(list_next(&cur_fte->list_elem),
                               struct frame_table_entry, list_elem);
}

bool frame_hash_less(const struct hash_elem* a, const struct hash_elem* b, void* aux UNUSED){
    const struct frame_table_entry * ta = hash_entry(a, struct frame_table_entry, hash_elem);
    const struct frame_table_entry * tb = hash_entry(b, struct frame_table_entry, hash_elem);
    return ta->frame < tb->frame;
}

unsigned frame_hash(const struct hash_elem* e, void* aux UNUSED){
    struct frame_table_entry* t = hash_entry(e, struct frame_table_entry, hash_elem);
    return hash_bytes(&t->frame, sizeof(t->frame));
}
