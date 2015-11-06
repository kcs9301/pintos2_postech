#include "threads/frame.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include <stdio.h>
#include <string.h>

static uint32_t *ft;

void 
frame_table_init (size_t upages)
{
	ft = palloc_get_page (PAL_ZERO);
	*(ft+1) = upages;
	*(ft+2) = upages;
}

bool
ft_check (void)
{
	if (ft_usable (ft) < 1)
		PANIC ("Out of Pages from ft"); // change later
	else
		return true;
}

bool
ft_insert (void *fpage)
{
	if (ft_usable (ft) < 1)
		PANIC ("Out of Pages from ft");

	int i = 3;	// because of total & usable
	for (i; i < ft_using (ft) + 4 ;i++)
	{
		uint32_t info = *(ft+i);
		if (info & FTE_P == 0x0){
			*(ft+i) = * (uint32_t *) fpage | FTE_P;
			*(ft+2) -= 1;
			return true;
		}
	}
}

bool
ft_delete (void *fpage)
{
	int i = 3;
	int count = 0;
	for (i; count < ft_using (ft) ; i++)
	{
		uint32_t info = *(ft+i);
		if (info & FTE_P == 0x1){
			if ((info & FTE_ADDR) == fpage){
				*(ft+i) = FTE_P;
				*(ft+2) += 1;
				return true;
			}
			count ++;
		}
	}
	return false;
}



