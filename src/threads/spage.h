#ifndef VM
#define VM_PAGE_H

#include <list.h>
#include "threads/frame.h"

#define FILE 0
#define SWAP 1
#define MMAP 2

#define SP_ADDR 0xfffff000

struct spage_entry {
  int type;

  bool already_loaded;
  bool pinned;

  //lazy load
  struct file *myfile;
  off_t ofs;
  uint8_t *upage;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;
  
  // swap
  size_t index;
  
  struct list_elem s_elem;
};

void spage_table_init (void);
//void spage_table_destroy (void);

bool load_page (void *);
bool from_swap (struct spage_entry *);
bool from_file (struct spage_entry *);
bool file_elem_spage_table (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool writable);
bool mmap_elem_page_table(struct file *file, int32_t ofs, uint8_t *upage,
			    uint32_t read_bytes, uint32_t zero_bytes);
struct spage_entry* find_entry (void *);

#endif 

