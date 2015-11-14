#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>


/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002,             /* Zero page contents. */
    PAL_USER = 004              /* User page. */
  };

void palloc_init (size_t user_page_limit);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);


#include <debug.h>
#include <list.h>
#include <stdint.h>

#define FTE_ADDR 0xfffff000
#define FTE_P 0x1
#define FTE_R 0x2

void swap_init (void);
size_t swap_out (void *fpage);
void swap_in (size_t offset, void *fpage);

struct frame_entry
{
	void *fpage;
	struct thread *t;
	struct spage_entry *se;
	struct list_elem f_elem;
};


void frame_table_init (void);
void *frame_get_page (struct spage_entry *);
bool frame_insert (void *, struct spage_entry *);
bool
frame_to_swap (struct frame_entry *fe);
void *
frame_evict (void);

#endif /* threads/palloc.h */
