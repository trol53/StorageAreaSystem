#include "task_queue.h"

task_t* task_init(void (*func)(), void* arg){
    task_t* task = (task_t*)malloc(sizeof(task_t));

    if (task == NULL){
        printf("Memory alloc error\n");
        return NULL;
    }

    task->func = func;
    task->arg = arg;
    task->next = NULL;
    task->prev = NULL;
    return task;
}

task_queue_t* queue_init(task_t* task){
    task_queue_t* queue = (task_queue_t*)malloc(sizeof(task_queue_t));
    if (queue == NULL){
        printf("Memory alloc error\n");
        return NULL;
    }

    pthread_mutex_init(&(queue->mutex), NULL);
    pthread_mutex_init(&(queue->cond_mutex), NULL);
    pthread_cond_init(&(queue->cond_var), NULL);

    queue->head = task;
    queue->tail = task;
    return queue;
}

void push(task_queue_t** queue, task_t* task){
    task_queue_t* tmp = *queue;
    pthread_mutex_lock(&(tmp->mutex));
    pthread_mutex_lock(&(tmp->cond_mutex));
    if (tmp->tail == NULL){
        tmp->head = task;
        tmp->tail = task;
        pthread_cond_signal(&(tmp->cond_var));
        pthread_mutex_unlock(&(tmp->mutex));
        pthread_mutex_unlock(&(tmp->cond_mutex));
        return;
    }
    task_t* prev_tail = tmp->tail;
    task->next = prev_tail;
    prev_tail->prev = task;
    tmp->tail = task;
    pthread_cond_signal(&(tmp->cond_var));
    pthread_mutex_unlock(&(tmp->mutex));
    pthread_mutex_unlock(&(tmp->cond_mutex));
    return;
}

task_t* pop(task_queue_t** queue){
    task_queue_t* tmp = *queue;
    pthread_mutex_lock(&(tmp->mutex));
    if (tmp->head == NULL){
        pthread_mutex_unlock(&(tmp->mutex));
        return NULL;
    }
    
    task_t* prev_head = tmp->head;
    task_t* new_head = prev_head->prev;
    if (new_head != NULL){
        new_head->next = NULL;
    } else {
        tmp->tail = NULL;
    }
    tmp->head = new_head;
    pthread_mutex_unlock(&(tmp->mutex));
    return prev_head;
}

int is_empty(task_queue_t* queue){
    return queue->head ? 1 : 0;
}

task_t* front(task_queue_t* queue){
    return queue->head;
}