#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "queue.h"
#include "so_scheduler.h"

typedef int bool;
#define false 0
#define true 1

#define MAX_THREADS	500

#define NEW		0
#define READY		1
#define RUNNING		2
#define WAITING		3
#define TERMINATED	4

/* boolean to know if the init function has been called */
static bool scheduler_init;
static unsigned int quantum;
static unsigned int io;
/* number of threads */
static int threads_no;
/* holds all threads */
static struct Thread **threads;
/* holds active threads */
static struct thread_queue *queue;
/* holds the running thread */
static struct Thread *running_thread;

/* wrapper around thread function */
static void *thread_start(void *args)
{
	struct Thread *t = (struct Thread *)args;
	/* wait for thread then run handler */
	sem_wait(&t->thread_sem);
	t->handler(t->priority);
	/* peek the next thread to run */
	if (queue->size == 0) {
		sem_post(&t->thread_sem);
		return NULL;
	}
	struct Thread *next = peek(queue);

	running_thread = next;
	pop(queue);
	next->status = RUNNING;
	next->time = quantum;
	sem_post(&next->thread_sem);
	return NULL;
}

/* creates and initializes scheduler */
/* + time quantum for each thread */
/* + number of IO devices supported */
/* returns: 0 on success or negative on error */
int so_init(unsigned int time_quantum, unsigned int io_dev)
{
	/* check params */
	if (scheduler_init == true ||
		time_quantum <= 0 ||
		io_dev < 0 ||
		io_dev > SO_MAX_NUM_EVENTS)
		return -1;
	/* initialize global variables */
	scheduler_init = true;
	quantum = time_quantum;
	io = io_dev;
	threads_no = 0;
	running_thread = NULL;
	threads = malloc(MAX_THREADS * sizeof(Thread));
	if (threads == NULL)
		return -1;
	queue = create_queue();

	return 0;
}

/* creates a new so_task_t and runs it according to the scheduler */
/* + handler function */
/* + priority */
/* returns: tid of the new task if successful or INVALID_TID */
tid_t so_fork(so_handler *func, unsigned int priority)
{
	int rc;
	/* check params */
	if (func == NULL ||
		priority < 0 ||
		priority > SO_MAX_PRIO)
		return INVALID_TID;
	/* init thread */
	struct Thread *t = malloc(sizeof(struct Thread));

	t->priority = priority;
	t->handler = func;
	t->dev = -1;
	t->time = quantum;
	rc = pthread_create(&t->tid, NULL, &thread_start, (void *)t);
	if (rc != 0) {
		free(t);
		return INVALID_TID;
	}
	sem_init(&t->thread_sem, 0, 0);
	/* add thread to threads vector */
	threads[threads_no] = t;
	threads_no++;
	/* add thread to queue */
	t->status = READY;
	push(queue, t);

	/* if there is no running threads, make the thread */
	/* with the greatest priority run */
	if (running_thread == NULL) {
		t = peek(queue);
		pop(queue);
		t->time = quantum;
		t->status = RUNNING;
		running_thread = t;
		sem_post(&t->thread_sem);
	} else
		so_exec();  /* update running thread */

	return t->tid;
}

/* waits for an IO devices */
/* + device index */
/* returns: -1 if the device does not exist or 0 on success */
int so_wait(unsigned int io_dev)
{
	if (io_dev >= io)
		return -1;

	running_thread->dev = io_dev;
	running_thread->status = WAITING;
	/* update the running thread */
	so_exec();
	return 0;
}

/* signals an IO device */
/* + device index */
/* return the number of tasks woke or -1 on error */
int so_signal(unsigned int io_dev)
{
	int i, sum = 0;

	if (io_dev >= io)
		return -1;
	/* wake up all threads blocked by the io_dev device */
	for (i = 0; i < threads_no; ++i) {
		if (threads[i]->status == WAITING &&
		threads[i]->dev == io_dev) {
			sum++;
			threads[i]->dev = -1;
			threads[i]->status = READY;
			push(queue, threads[i]);
		}
	}
	/* update the running thread */
	so_exec();
	return sum;
}

/* consumes time on processor and updates */
/* the running thread */
void so_exec(void)
{
	struct Thread *t = running_thread;

	t->time--;
	struct Thread *next;
	/* wait for the last thread to finish */
	if (queue->size == 0)
		sem_post(&t->thread_sem);
	else {
		next = peek(queue);
		/* if no thread is running, the running thread is waiting */
		/* or terminated, or there is a thread with greater */
		/* priority, choose the next thread as the running thread */
		if (t == NULL ||
		t->status == WAITING ||
		t->status == TERMINATED ||
		t->priority < next->priority) {
			pop(queue);
			if (t->priority < next->priority) {
				t->status = READY;
				push(queue, t);
			}
			running_thread = next;
			next->status = RUNNING;
			next->time = quantum;
			sem_post(&next->thread_sem);
		/* if the quantum has expired for the running thread */
		} else if (t->time <= 0) {
			/* if there is a thread with grater priority */
			/* run it */
			if (t->priority == next->priority) {
				pop(queue);
				t->status = READY;
				push(queue, t);
				running_thread = next;
				next->status = RUNNING;
				next->time = quantum;
				sem_post(&next->thread_sem);
			/* else, schedule the current thread to run again */
			} else {
				t->time = quantum;
				sem_post(&t->thread_sem);
			}
		} else
			sem_post(&t->thread_sem);
	}

	sem_wait(&t->thread_sem);
}

/* destroys a scheduler */
/* free memory and join threads */
void so_end(void)
{
	int i;

	if (scheduler_init == false)
		return;

	scheduler_init = false;
	/* wait for all threads to finish */
	for (i = 0; i < threads_no; ++i)
		pthread_join(threads[i]->tid, NULL);
	/* free memory */
	for (i = 0; i < threads_no; ++i) {
		sem_destroy(&threads[i]->thread_sem);
		free(threads[i]);
	}
	free(threads);
	dealloc(queue);
}
