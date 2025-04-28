#include <toml.h>
#include <stdio.h>
#include <stdlib.h>
#include "toml_parse.h"




nodes_creds_t* get_creds(char *conf_filename){
    FILE *conf_file = NULL;
    nodes_creds_t* ret = malloc(sizeof(nodes_creds_t));
    ret->num_of_nodes = 0;
    int index = 1;
    char errbuff[200];
    
    conf_file = fopen(conf_filename, "r");
    toml_table_t *conf = toml_parse_file(conf_file, errbuff, 200);
    if (!conf){
        free(ret);
        return NULL;
    }

    toml_datum_t n_nodes = toml_int_in(conf, "num_of_nodes");
    if (!n_nodes.ok)
        return ret;
    
    ret->num_of_nodes = n_nodes.u.i;

    ret->ip = malloc(ret->num_of_nodes * sizeof(char*));
    ret->port = malloc(ret->num_of_nodes * sizeof(char*));

    while(1){
        char tmp_buff[100];
        sprintf(tmp_buff, "%s%d", "node", index);
        toml_table_t *creds = toml_table_in(conf, tmp_buff);
        if (!creds)
            break;
        toml_datum_t ip = toml_string_in(creds, "address");
        toml_datum_t port = toml_string_in(creds, "port");
        ret->ip[index - 1] = ip.u.s;
        ret->port[index - 1] = port.u.s;

        index++;
    }
    fclose(conf_file);

    return ret;
}
