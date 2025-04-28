#ifndef PARITY_H_
#define PARITY_H_

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define MAX_PATH 4096
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

void copy_file(FILE* from, FILE* to){
    char buf[100];
    while (!feof(from)){
        int len = fread(buf, 1, 100, from);
        fwrite(buf, 1, len, to); 
    }
}

void _get_parity(char *block_file_name, char *parity_file_name){
    char ptmp_filename[MAX_PATH];
    sprintf(ptmp_filename, "%s%s", parity_file_name, "_tmp");
    FILE *parity_rfile = fopen(parity_file_name, "rb");
    FILE *block_rfile = fopen(block_file_name, "rb");
    FILE *parity_wfile = fopen(ptmp_filename, "wb");
    if (!parity_rfile){
        printf("cant open file:%s\n", parity_file_name);
        return;
    }
    if (!block_rfile){
        printf("cant open file:%s\n", block_file_name);
        return;
    }
    if (!parity_wfile){
        printf("cant open file:%s\n", ptmp_filename);
        return;
    }
    struct stat st;
    stat(block_file_name, &st);
    size_t block_len = st.st_size;
    stat(parity_file_name, &st);
    size_t parity_len = st.st_size;
    char parity_tmp[100], block_tmp[100];
    for (int i = 0; i < MIN(block_len, parity_len);){
        int len_p = 0, len_b = 0;
        len_b = fread(block_tmp, 1, 100, block_rfile);
        len_p = fread(parity_tmp, 1, 100, parity_rfile);
        for (int j = 0; j < MIN(len_p, len_b); j++){
            parity_tmp[j] = parity_tmp[j] ^ block_tmp[j];
        }
        i += len_p;
        fwrite(parity_tmp, 1, len_p, parity_wfile);
    }
    if (!feof(parity_rfile)){
        while (!feof(parity_rfile)){
            size_t len = fread(parity_tmp, 1, 100, parity_rfile);
            fwrite(parity_tmp, 1, len, parity_wfile);
        }
    }
    fclose(parity_rfile);
    fclose(parity_wfile);
    fclose(block_rfile);
    remove(parity_file_name);

    rename(ptmp_filename, parity_file_name);

}

void get_parity(char *file_name, int num_of_blocks){
    char parity_file_name[MAX_PATH], fblock_file_name[MAX_PATH];
    sprintf(parity_file_name, "%s%s", file_name, "_parity");
    sprintf(fblock_file_name, "%s%s", file_name, "1");

    FILE *file_parity = fopen(parity_file_name, "wb");
    if (!file_parity){
        printf("cant open parity file\n");
        return;
    }
    FILE *first_block = fopen(fblock_file_name, "rb");
    if (!first_block){
        printf("cant open block file\n");
        return;
    }
    copy_file(first_block, file_parity);
    fclose(first_block);
    fclose(file_parity);

    for (int i = 2; i <= num_of_blocks; i++){
        char block_file_name[MAX_PATH];
        sprintf(block_file_name, "%s%d", file_name, i);
        _get_parity(block_file_name, parity_file_name);
    }
}

void get_file_by_blocks(const char *file_name, int num_of_blocks){
    char tmp[100];
    FILE* file = fopen(file_name, "wb");
    for(int i = 1; i <= num_of_blocks; i++){
        char block_file_name[MAX_PATH];
        size_t f_size, size_count = 0, tmp_count;
        sprintf(block_file_name, "%s%d", file_name, i);
        FILE* block_file = fopen(block_file_name, "rb");
        fseek(block_file, 0, SEEK_END); 
        f_size = ftell(block_file);
        fseek(block_file, 0, SEEK_SET);  
        while (1){
            tmp_count = fread(tmp, 1, 100, block_file);
            fwrite(tmp, tmp_count, 1, file);
            size_count += tmp_count;
            if (size_count >= f_size){
                break;
            }
        }
        fclose(block_file);
        remove(block_file_name);

    }
    fclose(file);
}

void get_blocks(char *file_name, float num_of_blocks, float file_size){

    FILE* file = fopen(file_name, "rb");
    if (!file){
        printf("cant open file: %s\n", file_name);
        return;
    }
    int size_of_block = (int)(ceil(file_size / num_of_blocks));

    char tmp[100];
    for (int i = 1; i <= (int)(num_of_blocks); i++){
        char tmp_filename[MAX_PATH];
        int tmp_read_size = 0;
        sprintf(tmp_filename, "%s%d", file_name, i);
        FILE* block_file = fopen(tmp_filename, "wb");
        if (!block_file){
            printf("cant open file: %s\n", tmp_filename);
            return;
        }
        for (int j = 0; j <= size_of_block;){
            if (feof(file))
                break;
                
            if (j + 100 > size_of_block){
                tmp_read_size = fread(tmp, 1, size_of_block - j, file);
                fwrite(tmp, 1, tmp_read_size, block_file);
                break;
            }
            tmp_read_size = fread(tmp, 1, 100, file);
            
            fwrite(tmp, 1, tmp_read_size, block_file);
            j += tmp_read_size;
        }
        fclose(block_file);
    }
    fclose(file);
    remove(file_name);
}

void get_file_by_parity(char *file_name, int num_of_blocks, int broken_block){
    char parity_file_name[MAX_PATH], block_file_name[MAX_PATH];
    sprintf(parity_file_name, "%s%s", file_name, "_parity");
    sprintf(block_file_name, "%s%d", file_name, broken_block);
    rename(parity_file_name, block_file_name);
    get_parity(file_name, num_of_blocks);
    rename(parity_file_name, block_file_name);
    get_file_by_blocks(file_name, num_of_blocks);
}

#endif


