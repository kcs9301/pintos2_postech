#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_exit_only (struct process *);
int get_status (struct process *);
void process_activate (void);

struct process
{
	tid_t pid;

	int status;
	struct list_elem child_elem;
	uint32_t *pagedir;

	bool im_exit;

	struct semaphore sema_wait; // for my parent

	struct thread *mythread;

	bool my_parent_die;

	bool load_complete;
    bool load_success;
};

#endif /* userprog/process.h */
