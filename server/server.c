#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <liburing.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../utils/threadpool.h"
#include "../utils/ParityCode.h"
#include "../utils/toml_parse.h"
#include "db_interface.h"



int NUM_BLOCKS = 2;
struct io_uring ring, ring_db;
char buff_db[1024][BLOCK_SIZE];






void set_task(void* data){
    struct parity_info* info = (struct parity_info*)data;
    struct file_info* f_info = malloc(sizeof(struct file_info));
    f_info->file_name = info->file_name;

    get_parity(info->file_name, info->nodes->num_of_nodes - 1);
    for (int i = 0; i < info->nodes->num_of_nodes; i++){
        int  server_fd, len, port;
        // struct sockaddr_in* s_addr = malloc(sizeof(struct sockaddr_in));

        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (server_fd < 0){
            perror("Can't create socket fd");
            return;
        }

        f_info->s_addr_in.sin_family = AF_INET;
        f_info->s_addr_in.sin_addr.s_addr = inet_addr(info->nodes->ip[i]);
        port = atoi(info->nodes->port[i]);
        printf("Ready to connect to %s:%d\n", info->nodes->ip[i], port);
        f_info->s_addr_in.sin_port = htons(port);
        len = sizeof(f_info->s_addr_in);
        f_info->wfd = server_fd;
        f_info->block_read = i + 1;
        f_info->read_socket = 0;
        if (connect(server_fd, (struct sockaddr*)&f_info->s_addr_in, len) < 0){
            printf("Can't connect to %s:%d\nerror: %s\n", info->nodes->ip[i], port, strerror(errno));
            return;
        } else 
            printf("Connect to %s:%d success\n", info->nodes->ip[i], port);

        char block_filename[512];
                // if (u_data->info->block_read != u_data->info->)
        if (i != info->nodes->num_of_nodes)
            sprintf(block_filename, "%s%d", f_info->file_name, f_info->block_read);
        else
            sprintf(block_filename, "%s%s", f_info->file_name, "_parity");
        int fdf = open(block_filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if (!f_info->read_socket)
            f_info->rfd = fdf;
        else
            f_info->wfd = fdf;
        ring_add_read(&ring_db, f_info, BLOCK_SIZE, buff_db[f_info->rfd], 0);

    }
    free(info);
}



void read_header(int client_fd, char* filename, int* f_size, char* op){
    char ch;
    int num_read, total_read = 0, file_size = 0, r_type = 0;
    char* buf = filename;

    while (1){
        num_read = read(client_fd, &ch, 1);
        if (num_read == -1){
            printf("Cant read header\n");
            return;
        }
        if (num_read == 0){
            if (total_read == 0)
                return;
            else
                break;
        }
        if (ch == ' '){
            r_type = 1;
            continue;
        }
        if (ch == '\n')
            break;
        if (!r_type)
            file_size = file_size * 10 + (ch - '0');
        else
            *buf++ = ch;
        total_read++;
    }

    *f_size = file_size;
    *buf = '\0';
    return;
}


int main(){


    struct sockaddr_in server, client;
    int server_fd, accept_fd;
    char block_filename[512];
    int port = 1480, len;
    nodes_creds_t* creds = get_creds("/home/trol53/pets/DistributedDataBaseC/utils/config.toml");
    threadpool_t* threadpool = threadpool_init(4);
    if (threadpool == NULL)
        return 1;
    pthread_t db_thread;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        printf("Can't create socket fd\n");
        return 1;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    len = sizeof(server);

    if (bind(server_fd, (struct sockaddr*)&server, len) < 0){
        printf("Can't bind socket on port:%d\n", port);
        return 1;
    }
    if (listen(server_fd, 50) < 0){
        printf("Listen socket error\n");
        return 1;
    }

    struct io_uring_params params, params_db;
    
    memset(&params, 0, sizeof(params));
    memset(&params_db, 0, sizeof(params_db));
    

    assert(io_uring_queue_init_params(4096, &ring, &params) >= 0);
    assert(io_uring_queue_init_params(4096, &ring_db, &params_db) >= 0);

    pthread_create(&db_thread, NULL, db_connection, &ring_db);

    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }
    struct file_info fds[1024];
    char buff[1024][BUFF_SIZE];
    len = sizeof(client);
    ring_add_accept(&ring, server_fd, (struct sockaddr *)&client, &len);
    
    while (1){

        struct io_uring_cqe *cqe;
        int ret;

        io_uring_submit(&ring);

        ret = io_uring_wait_cqe(&ring, &cqe);
        struct io_uring_cqe *cqes[256];
        int cqe_count = io_uring_peek_batch_cqe(&ring, cqes, sizeof(cqes) / sizeof(cqes[0]));
        for (int i = 0; i < cqe_count; i++){
            cqe = cqes[i];
            size_t res = cqe->res;

            user_data_t *u_data = (user_data_t *) io_uring_cqe_get_data(cqe);
            unsigned type = u_data->type;
            switch (type)
            {
            case ACCEPT:
                printf("Accept new connection\n");
                if (res <= 0){
                    printf("Accept failed\n");
                    ring_add_accept(&ring, server_fd, (struct sockaddr *)&client, &len);
                    break;
                }

                int f_size;
                char op;
                struct file_info info;
                info.rfd = res;
                info.file_name = malloc(512);
                read_header(res, info.file_name, &f_size, &op);
                info.block_size = (int)(ceil((float)f_size / (float)(creds->num_of_nodes - 1)));
                printf("file %s, size:%d, block size:%ld\n", info.file_name, f_size, info.block_size);
                sprintf(block_filename, "%s%d", info.file_name, 1);
                int fdf = open(block_filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                info.wfd = fdf;
                fds[res] = info;

                ring_add_read(&ring, &fds[res], BUFF_SIZE, buff[res], 0);
                ring_add_accept(&ring, server_fd, (struct sockaddr *)&client, &len);
                break;
            case READ:
                if (res <= 0){
                    close(u_data->info->rfd);
                    close(u_data->info->wfd);
                    struct parity_info* info = malloc(sizeof(struct parity_info));
                    info->file_name = u_data->info->file_name;
                    info->nodes = creds;
                    
                    push_task(set_task, (void*)info);
                    break;
                }
                u_data->info->block_read += res;
                if (u_data->info->block_read > u_data->info->block_size){
                    ring_add_write(&ring, u_data->info, u_data->info->block_size - u_data->info->woffset, buff[u_data->info->rfd], u_data->info->woffset, WRITE_LAST);
                } else{
                    ring_add_write(&ring, u_data->info, res, buff[u_data->info->rfd], u_data->info->woffset, WRITE);
                }
                break;
            case WRITE:
                if (res <= 0){
                    close(u_data->info->rfd);
                    close(u_data->info->wfd);
                    free(u_data->info->file_name);
                    break;
                }
                ring_add_read(&ring, u_data->info, BUFF_SIZE, buff[u_data->info->rfd], u_data->info->roffset);
                fds[u_data->info->rfd].woffset += res;
                break;
            case WRITE_LAST:
                sprintf(block_filename, "%s%d", u_data->info->file_name, 2);
                fdf = open(block_filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                close(u_data->info->wfd);
                u_data->info->wfd = fdf;
                u_data->info->woffset = 0;
                u_data->info->block_read = BUFF_SIZE - res;
                ring_add_write(&ring,u_data->info, BUFF_SIZE - res, buff[u_data->info->rfd] + res, u_data->info->woffset, WRITE);
                
                break;
            case CONNECT:
                break;
            default:
                break;
            }

            free(u_data);
            io_uring_cqe_seen(&ring, cqe);
        }
    }

}