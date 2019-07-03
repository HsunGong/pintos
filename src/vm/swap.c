//
// Created by yzh on 2019/6/23.
//

#include "swap.h"
#include <threads/malloc.h>

static struct list swap_free_list;
const int PAGE_BLOCK_CNT = PGSIZE / BLOCK_SECTOR_SIZE;
struct block* swap_block;
swap_address top_sa = 0;

void swap_init(){
    swap_block = block_get_role(BLOCK_SWAP);
    ASSERT(swap_block != NULL);
    list_init(&swap_free_list);
}

swap_address swap_store(void *pa){
    swap_address sa = swap_find_entry();
    if (sa == (swap_address)-1)
        return sa;
    for (int i = 0; i < PAGE_BLOCK_CNT; i++){
        block_write(swap_block, sa + i, pa + i * BLOCK_SECTOR_SIZE);
    }
    return sa;
}

void swap_load(swap_address sa, void *pa){
    ASSERT(sa != (swap_address)-1);
    ASSERT(sa % PAGE_BLOCK_CNT == 0);
    ASSERT(is_kernel_vaddr(pa));

    for (int i = 0; i < PAGE_BLOCK_CNT; i++){
        block_read(swap_block, sa + i, pa + i * BLOCK_SECTOR_SIZE);
    }
    swap_free(sa);
}

void swap_free(swap_address sa){
    ASSERT(sa % PAGE_BLOCK_CNT == 0);

    if (top_sa == sa + PAGE_BLOCK_CNT)
        top_sa = sa;
    else{
        struct swap_entry* st = malloc(sizeof(struct swap_entry));
        st->sa = sa;
        list_push_back(&swap_free_list, &st->elem);
    }
}


swap_address swap_find_entry(){
    swap_address sa = (swap_address)-1;
    if (list_empty(&swap_free_list)){
        if (top_sa + PAGE_BLOCK_CNT < block_size(swap_block)){
            sa = top_sa;
            top_sa += PAGE_BLOCK_CNT;
        }
    }
    else{
        struct swap_entry* t = list_entry(list_front(&swap_free_list), struct swap_entry, elem);
        list_remove(&t->elem);
        sa = t->sa;
        free(t);
    }
    return sa;
}
