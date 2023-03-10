#ifndef _UTILS_H
#define _UTILS_H

#include <pthread.h>
#include <stdbool.h>

#include "so_scheduler.h"

/* Useful macro for handling error codes */
#define ASSERT(cond, exec) do { if (!(cond)) exec; } while (0)

typedef struct thread {
	unsigned int time_quantum;
	unsigned int priority;
	pthread_t tid;
	pthread_mutex_t mutex_pl;
	so_handler *handler;

	struct thread *next;
} Thread;

typedef struct Scheduler {
	bool init;
	unsigned int time_quantum;
	unsigned int io;

	Thread **readyQueue;

	Thread **blockingQueue;

	Thread *running;

	Thread *threads;
} Scheduler;

void destroy_thread(Thread *thread);

void insertQueue(Thread **queue, Thread *ti, int loc);

void insertList(Thread *ti);

Thread *extractPriorityQueue(void);

Thread *extractList(Thread **queue, unsigned int io);
#endif /* _UTILS_H */
