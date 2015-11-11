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

/////////////////////////////////////////////////////////////////////////////////
/*

31                       12   11         1 0
      Physical Page         |            R P

entry 1 is always the number of total pages
entry 2 is always the number of usable pages





*/
/////////////////////////////////////////////////////////////////////////////////

void frame_table_init (size_t);
bool ft_check (void);
bool ft_insert (void *);
bool ft_delete (void *);


#endif /* threads/palloc.h */
