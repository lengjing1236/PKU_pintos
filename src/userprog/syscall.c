#include "userprog/syscall.h"
#include "lib/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
static struct lock filesys_lock;    // 全局文件系统锁

void
syscall_init (void) 
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


// 封装exit函数以供多个地方使用（正常/异常退出）
static void
exit_with_status (int status)
{
  struct thread *cur = thread_current ();
  cur->self_stat->exit_status = status;
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit ();
}

/* ---------- user ptr helpers ---------- */
static void
check_uaddr (const void *uaddr)
{
  if (uaddr == NULL || !is_user_vaddr (uaddr) ||
      pagedir_get_page (thread_current ()->pagedir, uaddr) == NULL)
    exit_with_status (-1);
}

static int
get_user_byte (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

static void
check_ubuf (const void *buf, unsigned size)
{
  const uint8_t *p = buf;
  unsigned i;
  for (i = 0; i < size; i++) {
    if (get_user_byte (p + i) == -1) exit_with_status (-1);
  }
}

static void
check_cstr (const char *s)
{
  while (1) {
    int ch = get_user_byte ((const uint8_t *) s);
    if (ch == -1) exit_with_status (-1);
    if (ch == 0) break;
    s++;
  }
}

static int
fetch_int (const void *uaddr)
{
  check_uaddr (uaddr);
  check_uaddr ((const uint8_t *)uaddr + 3);
  return *(const int *)uaddr;
}

/* ---------- fd helpers ---------- */
static struct file_desc *
find_fd (int fd)
{
  struct thread *cur = thread_current ();
  struct list_elem *e;

  for (e = list_begin (&cur->fd_list); e != list_end (&cur->fd_list); e = list_next (e)) {
    struct file_desc *d = list_entry (e, struct file_desc, elem);
    if (d->fd == fd)
      return d;
  }
  return NULL;
}

static int
fd_get (struct file *f)
{
  struct thread *cur = thread_current ();
  struct file_desc *d = malloc (sizeof *d);
  if (!d)   // 内存不足
    return -1;
  
  d->fd = cur->next_fd++;
  d->file = f;
  list_push_back (&cur->fd_list, &d->elem);
  return d->fd;
}

static void
fd_close_one (int fd)
{
  struct file_desc *d = find_fd (fd);
  if (d == NULL) 
    return;

  file_close (d->file);
  list_remove (&d->elem);
  free (d);
}

static void
sys_halt (void)
{
  shutdown_power_off ();
}

static void
sys_exit (int status)
{
  exit_with_status (status);
}

static tid_t
sys_exec (const char *cmd_line)
{
  check_cstr (cmd_line);
  return process_execute (cmd_line);
}

static int
sys_wait (tid_t tid)
{
  return process_wait (tid);
}

static bool
sys_create (const char *file, unsigned initial_size)
{
  check_cstr (file);

  lock_acquire (&filesys_lock);
  bool success = filesys_create (file, initial_size);
  lock_release (&filesys_lock);
  return success;
}

static bool
sys_remove (const char *file)
{
  check_cstr (file);

  lock_acquire (&filesys_lock);
  bool success = filesys_remove (file);
  lock_release (&filesys_lock);
  return success;
}

static int
sys_open (const char *file)
{
  check_cstr (file);

  lock_acquire (&filesys_lock);
  struct file *f = filesys_open (file);
  if (f == NULL)
  {
    lock_release (&filesys_lock);
    return -1;
  }
  int fd = fd_get (f);
  if (fd == -1) file_close (f);
  lock_release (&filesys_lock);
  return fd;
}

static int
sys_filesize (int fd) 
{
  if (fd < 2) return -1;
  struct file_desc *d = find_fd (fd);
  if (d == NULL) return -1;
  lock_acquire (&filesys_lock);
  int len = file_length (d->file);
  lock_release (&filesys_lock);
  return len;
}

static int
sys_read (int fd, void *buffer, unsigned size)
{
  check_uaddr (buffer);
  check_ubuf (buffer, size);

  if (fd == STDIN_FILENO) {
    unsigned i;
    uint8_t *buf = buffer;
    for (i = 0; i < size; i++) buf[i] = input_getc ();
    return (int) size;
  }

  if (fd == STDOUT_FILENO) return -1;

  struct file_desc *d = find_fd (fd);
  if (d == NULL) return -1;

  lock_acquire (&filesys_lock);
  int n = file_read (d->file, buffer, size);
  lock_release (&filesys_lock);
  return n;
}

static int
sys_write (int fd, const void *buffer, unsigned size)
{
  check_ubuf (buffer, size);

  if (fd == STDOUT_FILENO) {
    putbuf (buffer, size);
    return (int) size;
  }

  if (fd == STDIN_FILENO) return -1;

  struct file_desc *d = find_fd (fd);
  if (d == NULL) return -1;

  lock_acquire (&filesys_lock);
  int n = file_write (d->file, buffer, size);
  lock_release (&filesys_lock);
  return n;
}

static void
sys_seek (int fd, unsigned position)
{
  if (fd < 2) return;
  struct file_desc *d = find_fd (fd);
  if (d == NULL) return;
  lock_acquire (&filesys_lock);
  file_seek (d->file, position);
  lock_release (&filesys_lock);
}

static unsigned
sys_tell (int fd)
{
  if (fd < 2) return 0;
  struct file_desc *d = find_fd (fd);
  if (d == NULL) return 0;
  lock_acquire (&filesys_lock);
  unsigned pos = file_tell (d->file);
  lock_release (&filesys_lock);
  return pos;
}

static void
sys_close (int fd)
{
  if (fd < 2) return;
  lock_acquire (&filesys_lock);
  fd_close_one (fd);
  lock_release (&filesys_lock);
}


static void
syscall_handler (struct intr_frame *f)
{
  int nr = fetch_int (f->esp);

  switch (nr) {
    case SYS_HALT:
      sys_halt ();
      break;
    case SYS_EXIT:
      sys_exit (fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_EXEC:
      f->eax = sys_exec ((const char *) fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_WAIT:
      f->eax = sys_wait ((tid_t) fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_CREATE:
      f->eax = sys_create ((const char *) fetch_int ((uint8_t *)f->esp + 4),
                           (unsigned) fetch_int ((uint8_t *)f->esp + 8));
      break;
    case SYS_REMOVE:
      f->eax = sys_remove ((const char *) fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_OPEN:
      f->eax = sys_open ((const char *) fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_FILESIZE:
      f->eax = sys_filesize (fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_READ:
      f->eax = sys_read (fetch_int ((uint8_t *)f->esp + 4),
                         (void *) fetch_int ((uint8_t *)f->esp + 8),
                         (unsigned) fetch_int ((uint8_t *)f->esp + 12));
      break;
    case SYS_WRITE:
      f->eax = sys_write (fetch_int ((uint8_t *)f->esp + 4),
                          (const void *) fetch_int ((uint8_t *)f->esp + 8),
                          (unsigned) fetch_int ((uint8_t *)f->esp + 12));
      break;
    case SYS_SEEK:
      sys_seek (fetch_int ((uint8_t *)f->esp + 4),
                (unsigned) fetch_int ((uint8_t *)f->esp + 8));
      break;
    case SYS_TELL:
      f->eax = sys_tell (fetch_int ((uint8_t *)f->esp + 4));
      break;
    case SYS_CLOSE:
      sys_close (fetch_int ((uint8_t *)f->esp + 4));
      break;
    default:
      exit_with_status (-1);
      break;
  }
}
  


