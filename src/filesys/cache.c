#include <stdbool.h>
#include <stdio.h>
#include "filesys.h"
#include "cache.h"

static struct cache_block cache[CACHE_SIZE];
static struct lock lock_cache_all;

int cnt_used, current_cache;

static int cache_get_free_cache()
{
  if (cnt_used == 0)
  {
    current_cache = 1;
    return 0;
  }

  while (cache[current_cache].used && cache[current_cache].accessed)
  {
    cache[current_cache].accessed = 0;
    current_cache = (current_cache + 1) % CACHE_SIZE;
  }

  if (cache[current_cache].used && cache[current_cache].dirty)
    block_write(fs_device, cache[current_cache].sector_idx, cache[current_cache].buf);
  if (cache[current_cache].used)
  {
    cache[current_cache].used = false;
    --cnt_used;
  }

  int ret = current_cache;
  current_cache = (current_cache + 1) % CACHE_SIZE;
  return ret;
}

void cache_init()
{
  lock_init(&lock_cache_all);
  cnt_used = 0;
  current_cache = -1;
  for (int i = 0; i < CACHE_SIZE; ++i)
  {
    cache[i].dirty = cache[i].used = false;
    cache[i].accessed = 0;
    memset(cache[i].buf, 0, BLOCK_SECTOR_SIZE);
  }
}

void cache_finish()
{
  lock_acquire(&lock_cache_all);

  for (int i = 0; i < CACHE_SIZE; ++i)
    if (cache[i].used && cache[i].dirty)
    {
      block_write(fs_device, cache[i].sector_idx, cache[i].buf);
      cache[i].used = false;

      --cnt_used;
    }

  lock_release(&lock_cache_all);
}

void cache_read(block_sector_t sector, void *buffer)
{
  lock_acquire(&lock_cache_all);
  bool found = false;
  for (int i = 0; i < CACHE_SIZE; ++i)
    if (cache[i].used && cache[i].sector_idx == sector)
    {

      memcpy(buffer, cache[i].buf, BLOCK_SECTOR_SIZE);
      ++cache[i].accessed;

      found = true;
      lock_release(&lock_cache_all);
      break;
    }
  if (found)
    return;
  int index = cache_get_free_cache();
  ++cnt_used;

  cache[index].sector_idx = sector;
  cache[index].dirty = false;
  cache[index].used = true;
  cache[index].accessed = 1;
  block_read(fs_device, sector, cache[index].buf);
  memcpy(buffer, cache[index].buf, BLOCK_SECTOR_SIZE);

  lock_release(&lock_cache_all);
}

void cache_write(block_sector_t sector, const void *buffer)
{
  lock_acquire(&lock_cache_all);
  bool found = false;
  for (int i = 0; i < CACHE_SIZE; ++i)
    if (cache[i].used && cache[i].sector_idx == sector)
    {

      memcpy(cache[i].buf, buffer, BLOCK_SECTOR_SIZE);
      cache[i].dirty = true;
      ++cache[i].accessed;

      found = true;
      lock_release(&lock_cache_all);
      break;
    }
  if (found)
    return;
  int index = cache_get_free_cache();
  ++cnt_used;

  cache[index].sector_idx = sector;
  cache[index].dirty = true;
  cache[index].used = true;
  cache[index].accessed = 1;
  memcpy(cache[index].buf, buffer, BLOCK_SECTOR_SIZE);

  lock_release(&lock_cache_all);
}
