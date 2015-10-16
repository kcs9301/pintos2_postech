#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
//#include "userprog/pagedir.c"

static void syscall_handler (struct intr_frame *);
static bool check_valid_pointer (void *);
static void exit (int ); // 1
static int write (int, const void *, unsigned); // 9

static struct lock filesys_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&filesys_lock);

}

static void
syscall_handler (struct intr_frame *f ) 
{
//	printf("system call!\n");

	void *esp = f->esp;

	if (!check_valid_pointer (esp)){
		f->vec_no = 14;
		intr_handler (f);
	}

	int syscall_number = *(int *) esp;

//	printf ("system call number : %d \n", syscall_number);

	switch (syscall_number){
		case 1 : {
			exit (0);
		}
		case 9 : {
			esp += 4;
			int fd = * (int *) esp;
			esp += 4;
			void *buffer = * (void **) esp;
			esp += 4;
			unsigned size = * (unsigned *) esp;
			esp -= 12;
			lock_acquire (&filesys_lock);
			write (fd, buffer, size);
			lock_release (&filesys_lock);
		}
	}
	return;

}

static bool
check_valid_pointer (void *esp)
{
	int *esp_i = (int *) esp;

	if (esp == NULL){
		printf ("esp is NULL pointer\n");
		return false;
	}
	if (!is_user_vaddr (*esp_i)){
		printf ("esp is kernel area\n");
		return false;
	}
	if (esp < 0){
		printf ("esp is negative value\n");
		return false;
	}
	if (*esp_i == 0){
		printf (" *esp is 0\n");
		return false;
	}
	return true;
}

static void exit (int status){ // add more
	struct thread *cur = thread_current ();

	printf ("%s: exit(%d)\n", cur->name, status);

	thread_exit ();
}

static int // 9
write (int fd, const void *buffer, unsigned size){ // add more
	if (fd == 1)
		putbuf (buffer, size);
	return size;
}