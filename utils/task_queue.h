#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

struct task_t{
    void (*func)(void*);
    void* arg;
    struct task_t* next;
    struct task_t* prev;
};

typedef struct task_t task_t;

typedef struct {
    task_t* head;
    task_t* tail;
    pthread_mutex_t mutex;
    pthread_mutex_t cond_mutex;
    pthread_cond_t cond_var;
} task_queue_t;

task_t* task_init(void (*func)(void*), void* arg);

task_queue_t* queue_init(task_t* task);

void push(task_queue_t** queue, task_t* task);

task_t* pop(task_queue_t** queue);

task_t* front(task_queue_t* queue);

int is_empty(task_queue_t* queue);


#endif