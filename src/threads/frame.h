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