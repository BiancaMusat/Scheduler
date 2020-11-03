#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "queue.h"

/* create priority queue for thread scheduling */
struct thread_queue *create_queue()
{
	struct thread_queue *th = malloc(sizeof(thread_queue));

	th->head = NULL;
	th->size = 0;
	return th;
}

/* return the element with highest priority */
Thread *peek(struct thread_queue *th)
{
	return th->head->t;
}

/* remove element with highest priority */
void pop(struct thread_queue *th)
{
	struct Elem *aux = th->head;

	th->head = aux->next;
	th->size--;
	free(aux);
}

/* add element to the priority queue */
void push(struct thread_queue *th, struct Thread *t)
{
	th->size++;
	/* if it is the first element in queue */
	if (th->size == 1) {
		th->head = malloc(sizeof(struct Elem));
		th->head->t = t;
		th->head->next = NULL;
	} else {
		struct Elem *aux = th->head;
		struct Elem *new = malloc(sizeof(struct Elem));

		new->t = t;
		/* if the new thread has the biggest priority, */
		/* make it the head of the queue */
		if (new->t->priority > aux->t->priority) {
			new->next = aux;
			th->head = new;
		} else {
			/* else, find where to add the thread and add it */
			struct Elem *prev = aux;

			aux = aux->next;
			while (aux != NULL &&
			new->t->priority <= aux->t->priority) {
				prev = aux;
				aux = aux->next;
			}
			new->next = aux;
			prev->next = new;
		}
	}
}

/* empty queue and free memory */
void dealloc(struct thread_queue *th)
{
	while (th->size > 0)
		pop(th);
	free(th);
}
