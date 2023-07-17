#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_config(char* ip, int* port) {
    FILE* file = fopen("./config.conf", "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", "./config.conf");
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != '#' && line[0] != '\n') {
            char key[64], value[64];
            sscanf(line, "%s = %s", key, value);
            if (strcmp(key, "ip") == 0) {
                strcpy(ip, value);
            }
            else if (strcmp(key, "port") == 0) {
                *port = atoi(value);
            }
        }
    }
    fclose(file);
}
