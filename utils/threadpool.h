#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <unistd.h>
#include "task_queue.h"

struct threadpool_t {
    int num_of_threads;
    pthread_t* threads;
};

typedef struct threadpool_t threadpool_t;

task_queue_t* queue;

void* empty_task(void*){
    while (1){
        // sleep(1);
        task_t* task = pop(&queue);
        if (task == NULL){
            printf("begin to wait\n");
            pthread_mutex_lock(&(queue->cond_mutex));
            pthread_cond_wait(&(queue->cond_var), &(queue->cond_mutex));
            pthread_mutex_unlock(&(queue->cond_mutex));
            continue;
        }
        task->func(task->arg);
        free(task);
    }
    return NULL;
}

threadpool_t* threadpool_init(int num_of_threads){
    threadpool_t* threadpool = (threadpool_t*)malloc(sizeof(threadpool_t));
    if (threadpool == NULL){
        printf("Memory alloc error\n");
        return NULL;
    }
    queue = queue_init(NULL);
    if (queue == NULL){
        free(threadpool);
        return NULL;
    }
    threadpool->num_of_threads = num_of_threads;
    threadpool->threads = (pthread_t*)malloc(num_of_threads * sizeof(pthread_t));
    for (int i = 0; i < num_of_threads; i++){
        pthread_create(&(threadpool->threads[i]), NULL, empty_task, NULL);
    }
    return threadpool;
}

void push_task(void (*func)(void*), void* arg){
    task_t* task = task_init(func, arg);
    push(&queue, task);
}

void threadpool_destroy(threadpool_t* tp){
    for (int i = 0; i < tp->num_of_threads; i++){
        // pthread_join(tp->threads[i], NULL);
        pthread_cancel(tp->threads[i]);
    }
}

#endif