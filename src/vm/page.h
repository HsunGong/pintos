//
// Created by yzh on 2019/6/23.
//

#ifndef PINTOS_PAGE_H
#define PINTOS_PAGE_H

#include "../lib/stdint.h"
#include "../lib/kernel/hash.h"
#include "../threads/thread.h"
#include "../threads/palloc.h"

#define PAGE_PAL_FLAG			0
#define PAGE_INST_MARGIN		32
#define PAGE_STACK_SIZE			0x800000
#define PAGE_STACK_LOWER_BOUND	(PHYS_BASE - PAGE_STACK_SIZE)

enum page_status {
    Frame, //in frame
    Swap, //in swap slot
    Disk //in disk file
};
typedef struct hash page_table;

struct page_table_entry {
    void* va; //virtual address
    enum page_status status;
    void* pa; //physical address or swap address or mmap handler
    void* fa; //file info
    bool dirty;
    struct hash_elem elem;
};

//find the entry of vAddr in page table
struct page_table_entry* page_find_entry(page_table* pt, void* va);

//for safety
struct page_table_entry* page_find_entry_with_lock(page_table* pt, void* va);

//create the page table
page_table* page_create();

//init page lock for thread/init.c
void page_init();

//free page_table, frame and swap slot
void page_free(page_table* pt);

//apply a frame for the page and create a new page table entry to map
bool page_fault_handler(const void* vaddr, bool dirty, void* esp);

//first check if there's already a pte for va, if not, create a new pte to map va and pa.
bool page_set_frame(void* va, void* pa, bool dirty);

//mmap a file and create a pte
bool page_mmap(page_table* pt, struct mmap_handler* mh, void* va);

//unmap a file
bool page_unmap(page_table* pt, void* va);

//evict a page
bool page_eviction(struct thread* curT, void* va, void* sa, bool toSwap);

//check if va is below stack underline and is available
bool page_available(page_table* pt, void* va);

bool page_hash_less(const struct hash_elem *lhs, const struct hash_elem *rhs, void *aux UNUSED);
unsigned page_hash(const struct hash_elem *e, void *aux UNUSED);
void page_destructor(struct hash_elem *e, void *aux UNUSED);
#endif //PINTOS_PAGE_H

