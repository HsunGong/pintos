//
// Created by yzh on 2019/6/24.
//

#ifndef PINTOS_FRAME_H
#define PINTOS_FRAME_H

#include "../lib/stdbool.h"
#include "../lib/kernel/hash.h"
#include "../lib/debug.h"
#include "../threads/palloc.h"

struct frame_table_entry{
    void *frame;
    void *va;
    struct thread* t;
    bool pinned; //if pinned, it cannot be evicted
    struct hash_elem hash_elem;
    struct list_elem list_elem;
};

//find the entry in the frame table
void* frame_find_entry(void* frame);

//apply a frame to be mapped from va
void* frame_apply(enum palloc_flags pFlag, void* va);

//replace a used frame if unable to apply new frame from user pool
void* frame_replace(void* va);

//init frame for thread/init.c
void  frame_init();

//free the frame
void  frame_free(void* frame);

//set frame pinned false and add it to clock_list if its pinned is previously true
bool frame_reset(void* frame);

//set currentFrame in clock list to next
void frame_clock_next();

bool frame_hash_less(const struct hash_elem* a, const struct hash_elem* b, void* aux UNUSED);
unsigned frame_hash(const struct hash_elem* e, void* aux UNUSED);

#endif //PINTOS_FRAME_H