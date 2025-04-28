#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define BUFF_SIZE 512

int main(int argc, char **argv){
    char* server_ip = "127.0.0.1";
    int port = 1480, server_fd, len, file_fd;
    struct sockaddr_in s_addr;
    char buff[BUFF_SIZE];
    size_t f_size, read_size = 0;

    char* filename = "picture.jpg";
    if (argc == 2){
        filename = argv[1];
    }
    struct stat st;
    stat(filename, &st);
    f_size = st.st_size;


    file_fd = open(filename, O_RDONLY);

    if (file_fd < 0){
        printf("Can't open file %s\n", filename);
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0){
        perror("Can't create socket fd");
        return 1;
    }

    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(server_ip);
    s_addr.sin_port = htons(port);
    len = sizeof(s_addr);

    if (connect(server_fd, (struct sockaddr*)&s_addr, len) < 0){
        printf("Can't connect to %s:%d\n", server_ip, port);
        return 1;
    }
    sprintf(buff, "%ld %s\n", f_size, filename);
    write(server_fd, buff, strlen(buff));
    while (read_size < f_size){
        int tmp_size = read(file_fd, buff, BUFF_SIZE);
        write(server_fd, buff, tmp_size);
        read_size += tmp_size;
    }

    close(file_fd);
    close(server_fd);

}