#include <string.h>
//#include <stdbool.h>
#include "filesys/file.h"
//#include "threads/interrupt.h"
//#include "threads/malloc.h"
//#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "threads/frame.h"
#include "threads/spage.h"

static bool
install_page (void *upage, void *kpage, bool writable);

void spage_table_init (void)
{
	list_init (&thread_current ()->spage_list);
}

bool load_page (void *upage)
{
	void *vpn = (uint32_t) upage & (uint32_t) SP_ADDR;
	struct spage_entry *se = find_entry (vpn);
	if (se == NULL)
		return false;

	switch (se->type){
		case 0: //lazy load (from file system)
			return from_file (se);
		case 1: //swap
//			return from_swap (se);
		case 2: //mmap
			return from_file (se);
		default :
			return false;
	}

}

bool from_file (struct spage_entry *se)
{
  struct thread *t = thread_current ();

  struct file *file = se->myfile;
  off_t ofs = se->ofs;
  uint8_t upage = se->upage;
  uint32_t read_bytes = se->read_bytes;
  uint32_t zero_bytes = se->zero_bytes;
  bool writable = se->writable;

  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

//  lock_acquire (&filesys_lock);
  file_seek (file, ofs);
//  lock_release (&filesys_lock);

  if ( !(read_bytes > 0 || zero_bytes > 0) )
    return false; 
    
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */  
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */  
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */  
//      lock_acquire (&filesys_lock);
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
//          lock_release (&filesys_lock);
          palloc_free_page (kpage);
          return false; 
        }
//          lock_release (&filesys_lock);
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */  
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

  return true;
}

bool file_elem_spage_table (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool writable)
{
	struct spage_entry *se = malloc (sizeof (struct spage_entry));
	if (se == NULL)
		return false;

	se->type = 0;
	se->already_loaded = false;
	se->pinned = false;
	se->myfile = file;
	se->ofs = ofs;
	se->upage = upage;
	se->read_bytes = read_bytes;
	se->zero_bytes = zero_bytes;
	se->writable = writable;

	list_push_back (&thread_current ()->spage_list, &se->s_elem);
	return true;

}


struct spage_entry *find_entry (void *vpn)
{
	struct list *sp_list = &thread_current ()->spage_list;
	struct list_elem *e;
	for(e = list_begin (sp_list); e != list_end (sp_list); e = list_next(e)){
		struct spage_entry *se = list_entry (e, struct spage_entry, s_elem);
		if (se->upage == vpn)
			return se;
	}
	return NULL;
}


static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}