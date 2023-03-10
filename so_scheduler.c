#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "so_scheduler.h"
#include "utils.h"

Scheduler scheduler;
Thread *last_thread;

void reschedule(void)
{
	// Decrement the running thread's time quantum
	scheduler.running->time_quantum--;
	int regulation = 0;

	// Extract the highest-priority thread from the ready queue
	Thread *extr = extractPriorityQueue();
	ASSERT(extr != NULL, return);

	// If the running thread's time quantum has reached 0 reset the time quantum
	if (scheduler.running->time_quantum == 0) {
		scheduler.running->time_quantum = scheduler.time_quantum;
		regulation = 1;
	}

	// Compare the extracted thread's priority to the running thread's priority, taking
	// the regulation value into account
	if (scheduler.running->priority < extr->priority + regulation) {
		Thread *thread = scheduler.running;

		// The extracted thread has higher priority, so insert the running thread
		// into the ready queue and swap it with the extracted thread
		insertQueue(scheduler.readyQueue, scheduler.running, scheduler.running->priority);
		scheduler.running = extr;

		// Unlock the running thread's mutex and lock the other thread's mutex
		pthread_mutex_unlock(&scheduler.running->mutex_pl);
		pthread_mutex_lock(&thread->mutex_pl);
	} else {
		// The extracted thread has lower priority, so just insert it back into
		// the ready queue at the front
		insertList(extr);
	}
}

int so_init(unsigned int time_quantum, unsigned int io)
{
	if (scheduler.init == true || io > SO_MAX_NUM_EVENTS || time_quantum < 1)
		return -1;

	scheduler.time_quantum = time_quantum;
	scheduler.io = io;

	// Mark the scheduler as initialized
	scheduler.init = true;

	// Allocate memory for the running thread
	scheduler.running = malloc(sizeof(Thread));
	ASSERT(scheduler.running != NULL, return -1);

	// Set the running thread's priority and time quantum
	scheduler.running->priority = 0;
	scheduler.running->time_quantum = scheduler.time_quantum;

	// Initialize the running thread's mutex
	pthread_mutex_init(&scheduler.running->mutex_pl, NULL);

	// Allocate memory for the ready queue and blocking queue
	scheduler.readyQueue = calloc(1 + SO_MAX_PRIO, sizeof(Thread *));
	scheduler.blockingQueue = calloc(io, sizeof(Thread *));

	return 0;
}

void so_end(void)
{
	if (scheduler.init == false)
		return;

	// Save the running thread in a local variable. It will be the last thread
	// and needs to be unlocked later.
	last_thread = scheduler.running;

	// Extract the highest-priority thread from the ready queue and set it as the new
	// running thread
	scheduler.running = extractPriorityQueue();

	// If the extraction was successful, unlock the new running thread's mutex and
	// lock the old running thread's mutex
	if (scheduler.running != NULL) {
		pthread_mutex_unlock(&scheduler.running->mutex_pl);
		pthread_mutex_lock(&last_thread->mutex_pl);
	}

	// Iterate over the list of all threads
	while (scheduler.threads != NULL) {
		Thread *thread = scheduler.threads;

		scheduler.threads = scheduler.threads->next;

		// Wait for the thread to finish executing and clean up its resources
		pthread_join(thread->tid, NULL);
		destroy_thread(thread);
	}

	// Destroy the last running thread and free its memory
	destroy_thread(last_thread);

	// Free the memory associated with the ready and blocking queues
	free(scheduler.readyQueue);
	free(scheduler.blockingQueue);
	scheduler.init = false;
}

void so_exec(void)
{
	ASSERT(scheduler.init == true || scheduler.running != NULL, return);

	reschedule();
}

int so_wait(unsigned int io)
{
	// Return -1 if the specified I/O event index is invalid
	if (io < 0 || io >= scheduler.io)
		return -1;

	// Insert the running thread into the blocking queue at the specified I/O event index
	insertQueue(scheduler.blockingQueue, scheduler.running, io);

	// Save a pointer to the running thread
	Thread *thread = scheduler.running;

	// Extract the highest-priority thread from the ready queue and set it as the new
	// running thread
	scheduler.running = extractPriorityQueue();

	// Unlock the new running thread's mutex and lock the old running thread's mutex
	pthread_mutex_unlock(&scheduler.running->mutex_pl);
	pthread_mutex_lock(&thread->mutex_pl);

	return 0;
}

void *start_thread(void *arg)
{
	Thread *thread = (Thread *)arg;

	// Lock the thread's mutex to synchronize access to its resources
	pthread_mutex_lock(&thread->mutex_pl);
	thread->handler(thread->priority);

	// Extract the highest-priority thread from the ready queue and set it as the
	// new running thread
	scheduler.running = extractPriorityQueue();

	// Add the current thread to the list of all threads and update its next pointer
	thread->next = scheduler.threads;
	scheduler.threads = thread;

	// If there is a new running thread, unlock its mutex
	// Otherwise, unlock the last thread's mutex
	if (scheduler.running != NULL)
		pthread_mutex_unlock(&scheduler.running->mutex_pl);
	else
		pthread_mutex_unlock(&last_thread->mutex_pl);

	return NULL;
}

int so_signal(unsigned int io)
{
	if (io < 0 || io >= scheduler.io)
		return -1;

	int nr = 0;

	// Extract all threads from the blocking queue at the specified I/O event index and
	// add them to the ready queue at their original priorities
	Thread *thread = extractList(scheduler.blockingQueue, io);

	while (thread != NULL) {
		insertQueue(scheduler.readyQueue, thread, thread->priority);
		nr++;
		thread = extractList(scheduler.blockingQueue, io);
	}

	// Reschedule to select the next thread to run
	reschedule();

	// Return the number of unblocked threads
	return nr;
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;

	// Allocate memory for a new Thread object
	Thread *thread = malloc(sizeof(Thread));

	ASSERT(thread != NULL, return INVALID_TID);

	// Unlock the running thread's mutex
	pthread_mutex_unlock(&scheduler.running->mutex_pl);

	// Initialize the new Thread object's fields
	thread->priority = priority;
	thread->time_quantum = scheduler.time_quantum;
	thread->handler = func;
	pthread_mutex_init(&thread->mutex_pl, NULL);
	pthread_mutex_lock(&thread->mutex_pl);

	// Create a new thread and insert the new Thread object into the ready queue
	pthread_create(&thread->tid, NULL, &start_thread, thread);
	insertQueue(scheduler.readyQueue, thread, priority);

	// Lock the running thread's mutex and reschedule
	pthread_mutex_lock(&scheduler.running->mutex_pl);
	reschedule();

	// Return the new thread's ID
	return thread->tid;
}

void destroy_thread(Thread *thread)
{
	// Destroy the thread's mutex
	pthread_mutex_destroy(&thread->mutex_pl);

	// Free the thread's memory
	free(thread);
}

void insertQueue(Thread **queue, Thread *ti, int loc)
{
	Thread **tl = &queue[loc];

	while (*tl)
		tl = &((*tl)->next);

	ti->next = *tl;
	*tl = ti;
}

void insertList(Thread *ti)
{
	Thread **tl = &scheduler.readyQueue[ti->priority];

	ti->next = *tl;
	*tl = ti;
}

Thread *extractPriorityQueue(void)
{
	Thread *thread = NULL;

	for (int i = SO_MAX_PRIO; i >= 0; i--) {
		if (scheduler.readyQueue[i] != NULL) {
			thread = scheduler.readyQueue[i];
			scheduler.readyQueue[i] = thread->next;
			break;
		}
	}

	return thread;
}

Thread *extractList(Thread **queue, unsigned int io)
{
	Thread *thread = queue[io];

	ASSERT(thread != NULL, return NULL);

	queue[io] = thread->next;
	thread->next = NULL;
	return thread;
}
