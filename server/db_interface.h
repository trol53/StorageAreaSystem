#ifndef DB_INTERFACE_H
#define DB_INTERFACE_H
#include "../utils/toml_parse.h"
#include "../utils/ring_utils.h"

extern char buff_db[1024][BLOCK_SIZE];

void* db_connection(void* data){
    struct io_uring* ring = (struct io_uring*)data;  
    char buff[1024][BUFF_SIZE];
    printf("Ready to db connect\n");

    while (1){
        struct io_uring_cqe *cqe;
        int ret;


        io_uring_submit(ring);


        ret = io_uring_wait_cqe(ring, &cqe);
        if (ret < 0){
            printf("Failed uring wait\n");
            return NULL;
        }
        struct io_uring_cqe *cqes[256];
        int cqe_count = io_uring_peek_batch_cqe(ring, cqes, sizeof(cqes) / sizeof(cqes[0]));
        for (int i = 0; i < cqe_count; i++){
            printf("Get batch in db interface\n");
            cqe = cqes[i];
            size_t res = cqe->res;

            user_data_t *u_data = (user_data_t *) io_uring_cqe_get_data(cqe);
            unsigned type = u_data->type;
            switch (type)
            {
            case CONNECT:
                printf("Connect to db\n");
                if (res < 0){
                    printf("Connect failed\n");
                    break;
                }

                char block_filename[512];
                sprintf(block_filename, "%s%d", u_data->info->file_name, u_data->info->block_read);
                int fdf = open(block_filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                if (!u_data->info->read_socket)
                    u_data->info->rfd = fdf;
                else
                    u_data->info->wfd = fdf;
                ring_add_read(ring, u_data->info, BLOCK_SIZE, buff[u_data->info->rfd], 0);
                break;
            case READ:
                if (res <= 0){
                    close(u_data->info->rfd);
                    close(u_data->info->wfd);
                    break;
                }
                if (!u_data->info->read_socket)
                    u_data->info->roffset += res;
                
                ring_add_write(ring, u_data->info, res, buff_db[u_data->info->rfd], u_data->info->woffset, WRITE);
                
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
                ring_add_read(ring, u_data->info, BUFF_SIZE, buff_db[u_data->info->rfd], u_data->info->roffset);
                break; 
            default:
                break;
            }

            free(u_data);
            io_uring_cqe_seen(ring, cqe);
        }
    }
    return NULL;
}


#endif