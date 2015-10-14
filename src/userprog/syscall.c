#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f ) 
{
	int *a = f->esp;

  printf ("system call!\n");
  printf ("vec_no : %x\n", f->vec_no);
  printf ("esp : %x\n", f->esp);
  printf ("a : %x\n", *a);
  
  thread_exit ();
}
