//
// Created by yzh on 2019/6/23.
//

#include <stdio.h>

#include "page.h"
#include "frame.h"
#include "swap.h"
#include "../threads/thread.h"
#include "../threads/synch.h"
#include "../threads/malloc.h"
#include "../threads/vaddr.h"
#include "../userprog/pagedir.h"
#include "../userprog/syscall.h"
#include "../lib/stddef.h"
#include "../lib/debug.h"

static struct lock page_lock;

void page_init() {
    lock_init(&page_lock);
}

struct page_table_entry* page_find_entry(page_table* pt, void* va) {
    ASSERT(pt != NULL);
    struct page_table_entry tmp;
    tmp.va = va;
    struct hash_elem* he = hash_find(pt, &(tmp.elem));
    return he == NULL? NULL : hash_entry(he, struct page_table_entry, elem);
}

struct page_table_entry* page_find_entry_with_lock(page_table* pt, void* va){
    lock_acquire(&page_lock);
    struct page_table_entry* pte = page_find_entry(pt, va);
    lock_release(&page_lock);
    return pte;
}

page_table* page_create() {
    page_table* pt = malloc(sizeof(page_table));
    if(pt != NULL) {
        if(hash_init(pt, page_hash, page_hash_less, NULL) == false) {
            free(pt);
            return NULL;
        }
        else
            return pt;
    }
    else
        return NULL;
}

void page_free(page_table* page_table) {
    lock_acquire(&page_lock);
    hash_destroy(page_table, page_destructor);
    lock_release(&page_lock);
}

bool page_fault_handler(const void* vaddr, bool dirty, void* esp) {
    struct thread* curT = thread_current();
    page_table* page_table = curT->page_table;
    uint32_t* pagedir = curT->pagedir;
    void* va = pg_round_down(vaddr);
    bool success = true;

    lock_acquire(&page_lock);

    struct page_table_entry* pte = page_find_entry(page_table, va);
    void* pa = NULL;

    ASSERT(is_user_vaddr(vaddr));
    ASSERT(!(pte != NULL && pte->status == Frame));

    //printf("vaddr:%p    esp:%p\n", vaddr, esp);

    if(dirty && pte != NULL && pte -> dirty == false)
        return false;

    if(va >= PAGE_STACK_LOWER_BOUND) {
        if(vaddr >= esp - PAGE_INST_MARGIN) {
            if(pte == NULL) {
                pa = frame_apply(PAGE_PAL_FLAG, va);
                if(pa == NULL)
                    success = false;
                else {
                    pte = malloc(sizeof(*pte));
                    pte->va = va;
                    pte->status = Frame;
                    pte->pa = pa;
                    pte->fa = NULL;
                    pte->dirty = true;
                    hash_insert(page_table, &pte->elem);
                }
            }
            else {
                switch(pte->status) {
                    case Swap:
                        pa = frame_apply(PAGE_PAL_FLAG, va);
                        if(pa == NULL) {
                            success = false;
                            break;
                        }
                        swap_load((swap_address) pte->pa, pa);
                        pte->pa = pa;
                        pte->status = Frame;
                        break;
                    default:
                        success = false;
                }
            }
        }
        else {
            success = false;
        }
    }
    else {
        if(pte == NULL)
            success = false;
        else {
            switch(pte->status) {
                case Swap:
                    pa = frame_apply(PAGE_PAL_FLAG, va);
                    if(pa == NULL) {
                        success = false;
                        break;
                    }
                    swap_load((swap_address)pte->pa, pa);
                    pte->pa = pa;
                    pte->status = Frame;
                    break;
                case Disk:
                    pa = frame_apply(PAGE_PAL_FLAG, va);
                    if(pa == NULL) {
                        success = false;
                        break;
                    }
                    mmap_read(pte->pa, va, pa);
                    pte->pa = pa;
                    pte->status = Frame;
                    break;
                default:
                    success = false;
            }
        }
    }

    frame_reset(pa);
    lock_release(&page_lock);
    if(success) {
        ASSERT(pagedir_set_page(pagedir, pte->va, pte->pa, pte->dirty));
    }
    return success;
}

bool page_set_frame(void* va, void* pa, bool dirty) {
    struct thread* curT = thread_current();
    page_table* pt = curT->page_table;
    uint32_t* pagedir = curT->pagedir;

    bool success = true;
    lock_acquire(&page_lock);

    struct page_table_entry* pte = page_find_entry(pt, va);
    if(pte == NULL) {
        pte = malloc(sizeof(*pte));
        pte->va = va;
        pte->status = Frame;
        pte->pa = pa;
        pte->fa = NULL;
        pte->dirty = dirty;
        hash_insert(pt, &pte->elem);
    }
    else
        success = false;

    lock_release(&page_lock);

    if(success) {
        ASSERT(pagedir_set_page(pagedir, pte->va, pte->pa, pte->dirty));
    }
    return success;
}

bool page_mmap(page_table* pt, struct mmap_handler* mh, void* va) {
    struct thread *curT = thread_current();
    bool success = true;
    lock_acquire(&page_lock);
    if(va < PAGE_STACK_LOWER_BOUND && page_find_entry(pt, va) == NULL) {
        struct page_table_entry *pte = malloc(sizeof(*pte));
        pte->va = va;
        pte->status = Disk;
        pte->pa = mh;
        pte->fa = mh;
        pte->dirty = mh->dirty;
        hash_insert(pt, &pte->elem);
    }
    else
        success = false;
    lock_release(&page_lock);
    return success;
}

bool page_unmap(page_table* pt, void* va) {
    struct thread *curT = thread_current();
    bool success = true;
    lock_acquire(&page_lock);

    if(va < PAGE_STACK_LOWER_BOUND && page_find_entry(pt, va) != NULL) {
        struct page_table_entry *pte = page_find_entry(pt, va);
        switch(pte->status) {
            case Disk:
                hash_delete(pt, &(pte->elem));
                free(pte);
                break;
            case Frame:
                if(pagedir_is_dirty(curT->pagedir, pte->va)) {
                    mmap_write(pte->fa, pte->va, pte->pa);
                }
                pagedir_clear_page(curT->pagedir, pte->va);
                hash_delete(pt, &(pte->elem));
                frame_free(pte->pa);
                free(pte);
                break;
            default:
                success = false;
        }
    }
    else
        success = false;
    lock_release(&page_lock);
    return success;
}

bool page_eviction(struct thread* curT, void* va, void* sa, bool toSwap) {
    struct page_table_entry *pte = page_find_entry(curT->page_table, va);
    bool success = true;

    if(pte != NULL && pte->status == Frame) {
        if(toSwap) {
            pte->pa = sa;
            pte->status = Swap;
        }
        else { //toFile
            ASSERT(pte->fa != NULL );
            pte->pa = pte->fa;
            pte->status = Disk;
        }
        pagedir_clear_page(curT->pagedir, va);
    }
    else
        success = false;
    return success;

}

bool page_hash_less(const struct hash_elem *lhs, const struct hash_elem *rhs, void* aux UNUSED) {
    return hash_entry(lhs, struct page_table_entry, elem)->va < hash_entry(rhs, struct page_table_entry, elem)->va;
}

unsigned page_hash(const struct hash_elem *e, void* aux UNUSED){
    struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
    return hash_bytes(&(pte->va), sizeof(pte->va));
}

void page_destructor(struct hash_elem *e, void* aux UNUSED) {
    struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);

    if(pte->status == Frame) {
        pagedir_clear_page(thread_current()->pagedir, pte->va);
        frame_free(pte->pa);
    }
    else if(pte->status == Swap)
        swap_free((swap_address) pte->pa);

    free(pte);
}

//check if the vaddr is available
bool page_available(page_table* pt, void* va) {
    return va < PAGE_STACK_LOWER_BOUND && page_find_entry(pt, va) == NULL;
}
