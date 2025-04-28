#ifndef TOML_PARSE_H
#define TOML_PARSE_H

struct nodes_creds {
    int num_of_nodes;
    char **ip;
    char **port;
} typedef nodes_creds_t;


nodes_creds_t* get_creds(char *conf_filename);

#endif