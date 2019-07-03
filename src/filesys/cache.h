#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/block.h"

void cache_init(void);
void cache_finish(void);
void cache_read(block_sector_t sector, void *buffer);
void cache_write(block_sector_t sector, const void *buffer);

#define CACHE_SIZE 64

struct cache_block
{
    unsigned char buf[BLOCK_SECTOR_SIZE];
    block_sector_t sector_idx;
    int accessed;
    bool dirty;
    bool used;
};

#endif /* filesys/cache.h */
