#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"
#include "threads/malloc.h"
#include "lib/user/syscall.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format(void);
static bool check_filedir_name(const char *name);


/*
 * Check whether a single file/dir name is valid
 * Only check the length and '/'
 * */
static bool check_filedir_name(const char *name)
{
  if (name == NULL)
    return false;
  for (int i = 0; i < READDIR_MAX_LEN + 1; i++)
  {
    if (name[i] == '/')
      return false;
    if (name[i] == '\0')
      return true;
  }
  return false;
}

/*
 * Check whether the name is '/'.
 * */
bool is_rootpath(const char *name)
{
  if (name == NULL)
    return false;
  if (name[0] == '/' && name[1] == '\0')
    return true;
  return false;
}

/*
 * Paser non root path to a form as:
 * previous directory + target file/dir name
 * is_dir is true if it's surely a dir, false if unknown.
 * if return true, prev_dir MUST be closed after use this function
 * */
bool path_paser(const char *target_path, struct dir **prev_dir, char **pure_name, bool *is_dir)
{
  *is_dir = false; // init it to false;
                   //  printf("%s\n", target_path);
  // copy the full path
  int length = strlen(target_path);
  if (length == 0)
    return false;
  char *path_copy = malloc(length + 1);
  strlcpy(path_copy, target_path, length + 1);

  // check the tail to check whether it is surely a dir.
  if (length > 0 && path_copy[length - 1] == '/')
  {
    *is_dir = true;
    length--;
    ASSERT(length > 0); // non-root dir, length shouldn't be 0 here.
    path_copy[length] = '\0';
  }

  // length 0 is invalid.
  if (length == 0)
  {
    free(path_copy);
    return false;
  }

  if (path_copy[0] == '/')
    *prev_dir = dir_open_root();
  else
    *prev_dir = dir_reopen(thread_current()->current_dir);

  //split token by '/'
  char *token, *save_ptr, *next_token;
  for (token = strtok_r(path_copy, "/", &save_ptr);; token = next_token)
  {
    if (!check_filedir_name(token))
    {
      free(path_copy);
      dir_close(*prev_dir);
      return false;
    }
    ASSERT(token != NULL);
    next_token = strtok_r(NULL, "/", &save_ptr);
    if (next_token == NULL) // token is the purename in path
    {
      strlcpy(*pure_name, token, READDIR_MAX_LEN + 1);
      break;
    }
    else
    {
      struct dir *tmp_dir = *prev_dir;
      *prev_dir = subdir_lookup(*prev_dir, token);
      dir_close(tmp_dir);
      if (*prev_dir == NULL)
      {
        free(path_copy);
        return false;
      }
    }
  }
  free(path_copy);
  return true;
}


/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void filesys_init(bool format)
{
  cache_init();

  fs_device = block_get_role(BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC("No file system device found, can't initialize file system.");

  inode_init();
  free_map_init();

  if (format)
    do_format();

  free_map_open();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void filesys_done(void)
{
  free_map_close();
  cache_finish();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool filesys_create(const char *name, off_t initial_size)
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root();
  bool success = (dir != NULL && free_map_allocate(1, &inode_sector) && inode_create(inode_sector, initial_size) && dir_add(dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release(inode_sector, 1);
  dir_close(dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open(const char *name)
{
  struct dir *dir = dir_open_root();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup(dir, name, &inode);
  dir_close(dir);

  return file_open(inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool filesys_remove(const char *name)
{
  struct dir *dir = dir_open_root();
  bool success = dir != NULL && dir_remove(dir, name);
  dir_close(dir);

  return success;
}

/* Formats the file system. */
static void
do_format(void)
{
  printf("Formatting file system...");
  free_map_create();
  if (!dir_create(ROOT_DIR_SECTOR, 16))
    PANIC("root directory creation failed");
  free_map_close();
  printf("done.\n");
}
