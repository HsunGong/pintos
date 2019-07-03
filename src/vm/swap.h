//
// Created by yzh on 2019/6/23.
//

#ifndef PINTOS_SWAP_H
#define PINTOS_SWAP_H

#include "../devices/block.h"
#include "../lib/kernel/hash.h"
#include <threads/vaddr.h>

typedef  block_sector_t swap_address;

struct swap_entry{
    swap_address sa;
    struct list_elem elem;
};

//get a swap slot
swap_address swap_find_entry();

//init swap for thread/init.c
void swap_init();

//store the frame(physical memory address) to a swap slot
swap_address swap_store(void *pa);

//load the swap slot to the frame
void swap_load(swap_address sa, void *pa);

//free the swap slot
void swap_free(swap_address sa);

#endif //PINTOS_SWAP_H
