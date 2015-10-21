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
void check_valid_pointer (void *);
void exit (int ); // 1
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

	check_valid_pointer ((const void *) f->esp);

	int *esp_i = (int *) esp;

	int syscall_number = *(int *) esp_i;

//	printf ("system call number : %d \n", syscall_number);

	switch (syscall_number){

		case 0 : {
			shutdown_power_off();
		}

		case 1 : {
			esp += 4;
			int status = * (int *) esp;
			esp -= 4;
			exit (status);
			break;
		}
		case 2 : {
			esp += 4;
			char *cmd_line = * (char **) esp;
			esp -= 4;
			f-> eax = exec (cmd_line);
			break;
		}
		case 3 : {
			esp += 4;
			tid_t pid = * (tid_t *) esp;
			esp -= 4;
			f-> eax = wait (pid);
			break;
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
			break;
		}
	}
	//exit (-1)
	return;
}

void check_valid_pointer (void *esp)
{
	if ( !is_user_vaddr (esp) || esp < 0 || esp == NULL )
	{
		barrier ();
		exit (-1);
	}
}


void exit (int status)
{ // 1
	struct thread *cur = thread_current ();
	struct list_elem *e;

	cur->myprocess->status = status;
	cur->myprocess->im_exit = true;

	if (status <-1 ){
		cur->myprocess->status = -1;
		thread_exit ();
	}

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
	if (!is_user_vaddr (cmd_line))
		exit (-1);

	char *fn_copy = palloc_get_page (0);
	strlcpy (fn_copy, cmd_line, PGSIZE);

	tid_t pid = process_execute (fn_copy);

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

