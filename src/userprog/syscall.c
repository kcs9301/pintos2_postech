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

struct file_d_elem * find_file (char *);

static void syscall_handler (struct intr_frame *);
void check_valid_stack_pointer (const void *);
void exit (int ); // 1
tid_t exec (const char *);	//2
int wait (tid_t ); //3
bool create (const char *file, unsigned initial_size); //4
bool remove (const char *file); //5
int open (const char * file); //6
int filesize (int ); //7
int read (int fd, void *buffer, unsigned size); //8
static int write (int, const void *, unsigned); // 9
void close (int ); //12


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

	check_valid_stack_pointer ((const void *) f->esp);	// write read open exec

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
			check_valid_stack_pointer (cmd_line);
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
		case 4 : {
			esp += 4;
			char *file = * (char **) esp;
			if (file == NULL)
				exit (-1);
			esp += 4;
			unsigned initial_size = * (unsigned *) esp;
			esp -= 8;
			check_valid_stack_pointer (file);
			f-> eax = create (file, initial_size);
			break;
		}
		case 5 : {
			esp += 4;
			char *file = * (char **) esp;
			esp -= 4;
			f->eax = remove (file);
			break;
		}
		case 6 : {
			esp += 4;
			char *file = * (char **) esp;
			esp -= 4;
			check_valid_stack_pointer (file);
			f-> eax = open (file);
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
			check_valid_stack_pointer (buffer);
			f->eax = write (fd, buffer, size);
//			lock_release (&filesys_lock);
			break;
		}
	}
	//exit (-1)
	return;
}

void check_valid_stack_pointer (const void *esp)
{
	// user address unmapped exit condition
	void *pointer = pagedir_get_page(thread_current()->pagedir, esp);

	if (pointer == NULL)
		exit (-1);

	if ( !is_user_vaddr (esp) || esp < 0 || esp == NULL || esp < 0x08048000)
	{
		//barrier ();
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

int wait (tid_t pid) //3
{
	return process_wait (pid);
}

bool 
create (const char *file, unsigned initial_size) //4
{
	bool succ;

	if (strlen (file) > 14)
		return false;

	lock_acquire (&filesys_lock);
	succ = filesys_create (file, initial_size);
	lock_release (&filesys_lock);

	if (succ)
	{
		struct file_d_elem *d = malloc (sizeof (struct file_d_elem));
		d->name = file;
		list_push_back (&file_all_list, &d->file_elem);
	}

	return succ;
}

bool
remove (const char *file) //5
{
	bool succ;

	lock_acquire (&filesys_lock);
	succ = filesys_remove (file);
	lock_release (&filesys_lock);

	if (succ)
	{
		struct file_d_elem *d = find_file (file);
		list_remove (d);
		free (d);
	}
}

int
open (const char *file) //6
{
	if (file == NULL)
		return -1;

	lock_acquire (&filesys_lock);
	struct file *f = filesys_open (file);
	lock_release (&filesys_lock);

	if (f == NULL)
		return -1;

	struct file_d_elem *reference = find_file (file);
	ASSERT (reference!=NULL);
	struct file_d_elem *d = malloc (sizeof (struct file_d_elem));
	ASSERT (d!=NULL);
	struct thread *t = thread_current ();

	d->name = reference->name;
	d->file = f;

	int fd = malloc (sizeof (int));
	fd = give_fd (t);

	d->fd = fd;
	list_push_back (&t->open_file_list, &d->thread_elem);

	return fd;
}

static int // 9
write (int fd, const void *buffer, unsigned size){ // add more
	if (fd == 1)
		putbuf (buffer, size);
	return size;
}

int 
filesize (int fd)
{
	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;
	else
		return file_length (d->file);

}

struct file_d_elem *
find_file (char *file)
{
	struct list_elem *e;
	for (e=list_begin (&file_all_list); e!=list_end (&file_all_list);
		e = list_next (e))
	{
		struct file_d_elem *d = list_entry (e, struct file_d_elem, file_elem);
		if (d->name == file)
			return d;
	}
}

struct file_d_elem *
search_file (int fd, struct thread *t)
{
	struct list_elem *e;
	for (e=list_begin (&t->open_file_list); e!=list_end (&t->open_file_list);
		e = list_next (e))
	{
		struct file_d_elem *d = list_entry (e, struct file_d_elem, thread_elem);
		if (d->fd == fd)
			return d;
	}	
	return NULL;
}

int
give_fd (struct thread *t)
{
	ASSERT (t!=NULL);

	int fd = 2;

	if (list_empty (&t->open_file_list))
		return fd;

	struct list_elem *e;
	for (e=list_begin (&t->open_file_list); e!=list_end (&t->open_file_list);
		e = list_next (e))
	{
		struct file_d_elem *d = list_entry (e, struct file_d_elem, thread_elem);
		ASSERT (d!=NULL)
		if (fd == d->fd)
			fd=fd+1;
	}
	return fd;
}