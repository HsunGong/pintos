#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);

#include "threads/thread.h"

void syscall_file_close(struct file *file);
struct file *syscall_file_open(const char *name);
bool syscall_translate_vaddr(const void *vaddr, bool write);

#endif /* userprog/syscall.h */
