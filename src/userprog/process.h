#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute(const char *file_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(void);

//for VM
struct mmap_handler* process_get_mmap_handler(mapid_t mapid);
bool process_delete_mmap_handler(struct mmap_handler* mh);

#endif /* userprog/process.h */
