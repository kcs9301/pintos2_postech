#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
//#include "userprog/pagedir.c"

static void syscall_handler (struct intr_frame *);
static bool check_valid_pointer (void *);
static void exit (int ); // 1
static int write (int, const void *, unsigned); // 9


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

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
			esp += 4;
			int status = * (int *) esp;
			esp -= 4;
			exit (status);
		}
		case 2 : {
			esp += 4;
			char *cmd_line = * (char **) esp;
			esp -= 4;
			f-> eax = exec (cmd_line);
		}
		case 3 : {
			esp += 4;
			tid_t pid = * (tid_t *) esp;
			esp -= 4;
			f-> eax = wait (pid);
		}
		case 9 : {
			esp += 4;
			int fd = * (int *) esp;
			esp += 4;
			void *buffer = * (void **) esp;
			esp += 4;
			unsigned size = * (unsigned *) esp;
			esp -= 12;
//			lock_acquire (&filesys_lock);
			f->eax = write (fd, buffer, size);
//			lock_release (&filesys_lock);
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

static void exit (int status){ // 1
	struct thread *cur = thread_current ();
	struct list_elem *e;

	cur->myprocess->status = status;
	cur->myprocess->im_exit = true;

	if (list_empty (&cur->child_list)){
		if (cur->myprocess->my_parent_die)
			thread_exit ();
		else
			thread_exit_only (cur); // it will become thread_exit_only
	}
	else {
		if (cur->myprocess->my_parent_die)
			thread_exit ();
		// visit all list entry ,and set the parent die true
		else {
			for (e = list_begin (&cur->child_list); e != list_end (&cur->child_list);
		    e = list_next (e)) {
  			  	struct process *p = list_entry (e, struct process, child_elem);
  				if (p->im_exit)
  					process_exit_only (p);
  				else
   	 				p->my_parent_die = true;
  				}
  				thread_exit_only (cur);
			}
	}
}

tid_t 
exec (const char *cmd_line)	//2
{
	tid_t pid = process_execute (cmd_line);

	if (pid == -1)
		return -1;

	if (get_load_complete (pid))
		return pid;
	else
		return -1;
}

int wait (tid_t pid)
{
	return process_wait (pid);
}

static int // 9
write (int fd, const void *buffer, unsigned size){ // add more
	if (fd == 1)
		putbuf (buffer, size);
	return size;
}

