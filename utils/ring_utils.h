#ifndef RING_UTILS_H
#define RING_UTILS_H
#include "toml_parse.h"
#include <sys/types.h>
#include <liburing.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>

#define BUFF_SIZE 512

struct file_info {
    int rfd;
    int wfd;
    int roffset;
    int woffset;
    size_t f_size;
    size_t block_size;
    struct sockaddr_in s_addr_in;
    char *file_name;
    int block_read;
    int read_socket;
};

struct parity_info {
    char* file_name;
    nodes_creds_t* nodes;
    int broken_block;
};

struct user_data {
    unsigned type;
    struct file_info* info;
} typedef user_data_t;

enum {
    ACCEPT,
    READ,
    WRITE,
    WRITE_LAST,
    CONNECT,
};

void ring_add_connect(struct io_uring *ring, struct file_info* finfo){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (sqe == NULL){
        printf("Failed to prep connect\n");
        return;
    } else
        printf("Prep to connect\n");
    int len = sizeof(finfo->s_addr_in);
    io_uring_prep_connect(sqe, finfo->wfd, (struct sockaddr*)&finfo->s_addr_in, len);
    user_data_t *u_data = (user_data_t *)malloc(sizeof(user_data_t));
    u_data->info = finfo;
    u_data->type = CONNECT;

    io_uring_sqe_set_data(sqe, u_data);
}

void ring_add_read(struct io_uring *ring, struct file_info* finfo, size_t size, void *buff, size_t offset){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_read(sqe, finfo->rfd, buff, size, offset);
    user_data_t *u_data = (user_data_t *)malloc(sizeof(user_data_t));
    u_data->info = finfo;
    u_data->type = READ;

    io_uring_sqe_set_data(sqe, u_data);
}

void ring_add_write(struct io_uring *ring, struct file_info* finfo, size_t size, void *buff, size_t offset, unsigned type){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_write(sqe, finfo->wfd, buff, size, offset);
    user_data_t *u_data = (user_data_t *)malloc(sizeof(user_data_t));
    u_data->info = finfo;
    u_data->type = type;

    io_uring_sqe_set_data(sqe, u_data);
}

void ring_add_accept(struct io_uring *ring, int server_fd, struct sockaddr * s_addr, int* len){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, server_fd, s_addr, (socklen_t*)len, 0);
    user_data_t *u_data = (user_data_t *)malloc(sizeof(user_data_t));
    u_data->type = ACCEPT;

    io_uring_sqe_set_data(sqe, u_data);
}

#endif