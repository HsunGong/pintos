#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"
#include <lib/string.h>
#include <lib/stddef.h>

void syscall_file_close(struct file *file);
struct file *syscall_file_open(const char *name);
bool syscall_translate_vaddr(const void *vaddr, bool write);

void syscall_init(void);

static void syscall_mmap(struct intr_frame* f, int fd, const void* obj_va);
static void syscall_munmap(struct intr_frame* f, mapid_t mapid);
void mmap_read(struct mmap_handler* mh, void* va, void* pa);
void mmap_write(struct mmap_handler* mh, void* va, void* pa);
bool mmap_load_segment(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
bool mmap_check_va(struct thread* curT, const void* va, int page_cnt);
bool mmap_install_page(struct thread* curT, struct mmap_handler* mh);
#endif /* userprog/syscall.h */
