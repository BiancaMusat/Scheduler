#ifndef QUEUE_H_
#define QUEUE_H_

#include "so_scheduler.h"

/* Thread structure */
typedef struct Thread {
	int status;
	int priority;
	tid_t tid;
	so_handler *handler;
	int dev;
	int time;
	sem_t thread_sem;
} Thread;

typedef struct Elem {
	Thread *t;
	struct Elem *next;
} Elem;

typedef struct thread_queue {
	struct Elem *head;
	int size;
} thread_queue;

struct thread_queue *create_queue();
Thread *peek(struct thread_queue *th);
void pop(struct thread_queue *th);
void push(struct thread_queue *th, struct Thread *t);
void dealloc(struct thread_queue *th);

#endif      /* QUEUE_H_ */
