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
#include "../utils/ring_utils.h"

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
}

int main(int argc, char** argv){


    struct sockaddr_in server, client;
    int server_fd, accept_fd;
    char block_filename[512];
    int port, len;

    if (argc == 2){
        port = atoi(argv[1]);
    }

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

    struct io_uring ring;
    struct io_uring_params params;
    
    memset(&params, 0, sizeof(params));


    assert(io_uring_queue_init_params(4096, &ring, &params) >= 0);



    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }
    struct file_info fds[1024];
    char buff[1024][BUFF_SIZE];
    len = sizeof(client);
    ring_add_accept(&ring, server_fd, (struct sockaddr *)&client, &len);
    printf("Ready to get connections\n");
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
                struct file_info* info = malloc(sizeof(struct file_info));
                info->rfd = res;
                info->file_name = malloc(512);
                read_header(res, info->file_name, &f_size, &op);
                info->read_socket = 1;
                int fdf = open(info->file_name, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                info->wfd = fdf;

                ring_add_read(&ring, info, BUFF_SIZE, buff[res], 0);
                ring_add_accept(&ring, server_fd, (struct sockaddr *)&client, &len);
                break;
            case READ:
                if (res <= 0){
                    close(u_data->info->rfd);
                    close(u_data->info->wfd);
                    break;
                }
                if (!u_data->info->read_socket)
                    u_data->info->roffset += res;
                
                ring_add_write(&ring, u_data->info, res, buff[u_data->info->rfd], u_data->info->woffset, WRITE);
                
                break;
            case WRITE:
                if (res <= 0){
                    close(u_data->info->rfd);
                    close(u_data->info->wfd);
                    free(u_data->info->file_name);
                    break;
                }
                if (u_data->info->read_socket)
                    u_data->info->woffset += res;
                ring_add_read(&ring, u_data->info, BUFF_SIZE, buff[u_data->info->rfd], u_data->info->roffset);
                break;
                
            default:
                break;
            }

            free(u_data);
            io_uring_cqe_seen(&ring, cqe);
        }
    }


}