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
struct file_d_elem *search_file (int , struct thread *);

static void syscall_handler (struct intr_frame *);
void check_valid_stack_pointer (const void *);
//void all_file_close (struct thread *);

void exit (int ); // 1
tid_t exec (const char *);	//2
int wait (tid_t ); //3
bool create (const char *file, unsigned initial_size); //4
bool remove (const char *file); //5
int open (const char * file); //6
int filesize (int ); //7
int read (int fd, void *buffer, unsigned size); //8
static int write (int, const void *, unsigned); // 9
void seek (int, unsigned); //10
unsigned tell (int); //11
static void close (int ); //12

bool
check_same_file (struct thread *t, char *file);


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
		case 7 : {	//filesize
			esp += 4;
			int fd = * (int *) esp;
			esp -= 4;
			f-> eax = filesize (fd);
			break;
		}
		case 8 : {	//read
			esp += 4;
			int fd = * (int *) esp;
			esp += 4;
			void *buffer = * (void **) esp;
			esp += 4;
			unsigned size = * (unsigned *) esp;
			esp -= 12;
			check_valid_stack_pointer (buffer);
			f->eax = read (fd, buffer, size);
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
		case 10 : {
			esp += 4;
			int fd = * (int *) esp;
			esp += 4;
			unsigned position = * (unsigned *)esp;
			esp -= 8;
			seek (fd, position);
			break;
		}
		case 11 : {
			esp += 4;
			int fd = * (int *) esp;
			esp -= 4;
			f->eax = tell (fd);
			break;
		}
		case 12 : {
			esp += 4;
			int fd = * (int *) esp;
			esp -= 4;
			close (fd);
			break;
		}
	}
	//exit (-1)
	return;
}

void check_valid_stack_pointer (const void *esp)
{

	if ( !is_user_vaddr (esp) || esp < 0 || esp == NULL || esp < 0x08048000)
	{
		//barrier ();
		exit (-1);
	}
	// user address unmapped exit condition
	void *pointer = pagedir_get_page(thread_current()->pagedir, esp);

	if (pointer == NULL)
		exit (-1);

}


void exit (int status)
{ // 1
	struct thread *cur = thread_current ();
	struct list_elem *e;

	cur->myprocess->status = status;
	cur->myprocess->im_exit = true;

	if (status <-1 )
		cur->myprocess->status = -1;

	if (list_empty (&cur->child_list)){
		if (cur->myprocess->my_parent_die){
//			all_file_close (cur);
			thread_exit ();
		}
		else{
//			all_file_close (cur);
			thread_exit_only (cur); // it will become thread_exit_only
//			sema_up (&cur->myprocess->sema_wait);
		}
	}
	else {
		if (cur->myprocess->my_parent_die){
//			all_file_close (cur);
			thread_exit ();
		}
		// visit all list entry ,and set the parent die true
		else {
			for (e = list_begin (&cur->child_list); e != list_end (&cur->child_list);
		    e = list_next (e)) {
  			  	struct process *p = list_entry (e, struct process, child_elem);
  				if (p->im_exit){
  //					all_file_close (p->mythread);
  					process_exit_only (p);
  				}
  				else
 // 					list_insert (&cur->myprocess->child_elem, &p->child_elem);
   	 				p->my_parent_die = true;
  				}
//  				all_file_close (cur);
  				thread_exit_only (cur);
 // 				sema_up (&cur->myprocess->sema_wait);
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
	{
		palloc_free_page (fn_copy);
		return pid;
	}
	else{
		palloc_free_page (fn_copy);
		return -1;
	}
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
/*
	if (succ)
	{
		struct file_d_elem *d = malloc (sizeof (struct file_d_elem));
		d->name = file;
		list_push_back (&file_all_list, &d->file_elem);
	}
*/
	return succ;
}

bool
remove (const char *file) //5
{
	bool succ;

	lock_acquire (&filesys_lock);
	succ = filesys_remove (file);
	lock_release (&filesys_lock);
/*
	if (succ)
	{
		struct file_d_elem *d = find_file (file);
		list_remove (d);
		free (d);
	}
*/
	return succ;

}

int
open (const char *file) //6
{
	if (file == NULL)
		return -1;

	struct file_d_elem *d = malloc (sizeof (struct file_d_elem));
	ASSERT (d!=NULL);

	struct thread *t = thread_current ();
	struct file *f;


	lock_acquire (&filesys_lock);
		f = filesys_open (file);
	lock_release (&filesys_lock);

	if (f == NULL)
		return -1;

	//struct file_d_elem *reference = find_file (file);
	//ASSERT (reference!=NULL);
	//if (reference == NULL)
	//	return -1;
	// Some files could exist before the program starts.


	d->name = file;
	d->file = f;

//	int fd = malloc (sizeof (int));
//	fd = give_fd (t);

	d->fd = give_fd (t); //fd;

	list_push_back (&t->open_file_list, &d->thread_elem);

	return d->fd;
}

int 
filesize (int fd) //7
{
	if (fd < 2)
		return -1;

	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;
	
	int return_value;
	lock_acquire (&filesys_lock);
	return_value = file_length (d->file);
	lock_release (&filesys_lock);
	return return_value;
}

int
read (int fd, void *buffer, unsigned size)	//8
{
	if (fd == 0)
		return input_getc();
	else if (fd == 1)
		return -1;

	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;
	int return_value;
	lock_acquire (&filesys_lock);
	return_value = file_read (d->file, buffer, size);
	lock_release (&filesys_lock);
	return return_value;

}

static int // 9
write (int fd, const void *buffer, unsigned size){ // add more
	if (fd == 1)
		putbuf (buffer, size);
	else if (fd == 0)
		return -1;

	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;

	int return_value;
	lock_acquire (&filesys_lock);
	return_value = file_write (d->file, buffer, size);
	lock_release (&filesys_lock);
	check (return_value);
	return return_value;
}

void
check (int returnv)
{
	int retu = returnv;
}

void
seek (int fd, unsigned position)	//10
{
	// No more detail in Project2

	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;
	lock_acquire (&filesys_lock);
	file_seek (d->file, position);
	lock_release (&filesys_lock);
}

unsigned
tell (int fd)	//11
{
	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	if (d==NULL)
		return -1;
	int return_value;
	lock_acquire (&filesys_lock);
	return_value = file_tell (d->file);
	lock_release (&filesys_lock);
	return return_value;
}

void
close (int fd)	//12
{
	struct thread *t = thread_current ();
	struct file_d_elem *d = search_file (fd, t);
	
	if (d == NULL)
		return -1;

//	list_remove (&d->file_elem);
	list_remove (&d->thread_elem);

	lock_acquire (&filesys_lock);
	file_close (d->file);
	lock_release (&filesys_lock);

	free (d);
}

// USELESS
struct file_d_elem *
find_file (char *file)	// among the all file list in process kernel
{
	struct list_elem *e;
	for (e=list_begin (&file_all_list); e!=list_end (&file_all_list);
		e = list_next (e))
	{
		struct file_d_elem *d = list_entry (e, struct file_d_elem, file_elem);
		if (d->name == file)
			return d;
	}
//	return NULL;
}

struct file_d_elem *
search_file (int fd, struct thread *t)	// among the thread 
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

bool
check_same_file (struct thread *t, char *file)
{
  struct list_elem *e;
  for (e=list_begin (&t->open_file_list); e!=list_end (&t->open_file_list);
    e = list_next (e)) 
  {
    struct file_d_elem *d = list_entry (e, struct file_d_elem, thread_elem);
    if (d->name == file)
    	return true;
  }
  return false;
}

