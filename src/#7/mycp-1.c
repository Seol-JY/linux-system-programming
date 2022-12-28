#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    switch (argc) {
    case 1:
        puts("mycp: missing file operand");
        puts("Try 'mycp --help' for more information.");
        exit(1);
    case 2:
        printf("mycp: missing destination file operand after '%s'\n", argv[1]);
        puts("Try 'mycp --help' for more information.");
        exit(1);
    case 3:
        FILE *operand = fopen(argv[1], "r");
        if (!operand) {
            printf("mycp: cannot stat '%s': %s", argv[1], strerror(errno));
            exit(1);
        }
        char buf[MAX_BUFFER_SIZE];
        FILE *destination = fopen(argv[2], "w");
        while ((fgets(buf, MAX_BUFFER_SIZE, operand))) {
            fputs(buf, destination);
        }

        fclose(operand);
        fclose(destination);
        return 0;
    }
}
