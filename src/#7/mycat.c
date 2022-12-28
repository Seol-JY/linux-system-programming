#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024
unsigned int line_num = 0;
int mycat(FILE *fp, unsigned char flag, int last_breakline);

int main(int argc, char *argv[]) {
    int opt;
    unsigned char flag = 0;
    while ((opt = getopt(argc, argv, "nE")) != -1) {
        /*
        flag = 0: no option
        flag = 1: -n
        flag = 2: -E
        flag = 3: -nE
        */
        switch (opt) {
        case 'n': flag |= 1; break;
        case 'E': flag |= 2; break;
        case '?':
            puts("Try 'mycat --help' for more information.");
            exit(1);
        }
    }
    if (optind == argc) {
        /* stdin */
        mycat(stdin, flag, 0);
        return 0;
    }
    /* file */
    FILE * fp;
    int last_breakline = 1;
    for (int i = optind; i < argc; ++i) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            printf("mycat: %s: %s\n", argv[i], strerror(errno));
        } else {
            last_breakline = mycat(fp, flag, last_breakline); 
            fclose(fp);
        }
    }
    return 0;
}

int mycat(FILE *fp, unsigned char flag, int last_breakline) {
    char buf[MAX_BUFFER_SIZE];
    unsigned int tmp = 0;
    while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
        if (flag & 1) {
            if (last_breakline) { printf("%6d  ", ++line_num); }
            else { last_breakline = 1; }
        }
        if (flag & 2) {
            if (buf[strlen(buf)-1] == '\n') {
                tmp = strlen(buf);
                buf[tmp-1] = '$'; buf[tmp] = '\n'; buf[tmp+1] = '\0';
            }
        }
        fputs(buf, stdout);
    }
    /* 파일의 끝에 개행이 있는지 검사하여 반환, 개행 존재 시 1반환*/
    if (buf[strlen(buf)-1] == '$' || buf[strlen(buf)-1] == '\n') {
        return 1;
    } else { return 0; }
}