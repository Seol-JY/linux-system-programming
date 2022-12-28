#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_BUFFER_SIZE 1024

char buf[8192];
void CopySourceToDirectory(int argc, char * argv[], char* last_arg_name, struct stat last_arg_stat);
void CopySourceToDest(int argc, char * argv[], char* last_arg_name, struct stat last_arg_stat);

int main(int argc, char *argv[]) {
    char* last_arg_name;
    struct stat last_arg_stat;
    switch (argc) {
    case 1:
        puts("mycp: missing file operand");
        puts("Try 'mycp --help' for more information.");
        exit(1);
    case 2:
        printf("mycp: missing destination file operand after '%s'\n", argv[1]);
        puts("Try 'mycp --help' for more information.");
        exit(1);
    default: // argc > 2
        last_arg_name = argv[argc-1];
        lstat(last_arg_name, &last_arg_stat);
        if (S_ISDIR(last_arg_stat.st_mode)) {
            CopySourceToDirectory(argc, argv, last_arg_name, last_arg_stat);
        } else {
            CopySourceToDest(argc, argv, last_arg_name, last_arg_stat);
        }
    }
}

void CopySourceToDirectory(int argc, char * argv[], char* last_arg_name, struct stat last_arg_stat) {
    struct stat source_stat;
    char source_arg[1024];
    char source_path[1024];
    ssize_t length;

    for (int idx = 1; idx < argc-1; idx++) {

        lstat(argv[idx], &source_stat);
        if (S_ISDIR(source_stat.st_mode)) {
          printf("mycp: -r not specified; omitting directory '%s'\n", argv[idx]);
        }
        else if (access(argv[idx], R_OK)) {
          printf("mycp: cannot open '%s' for reading: Permission denied\n", argv[idx]);
        } 
        else if (access(last_arg_name, W_OK)) {
          printf("mycp: cannot create regular file '%s': Permission denied\n", argv[idx]);
        }
        else {
            if (S_ISLNK(source_stat.st_mode)) {
                readlink(argv[idx], source_arg, 1024);
                lstat(source_arg, &source_stat); // lstat을 원본으로 업데이트
            } else {
                strcpy(source_arg, argv[idx]);
            }
            sprintf(source_path, "%s/%s", last_arg_name, argv[idx]);

            if (opendir(source_path) != NULL) {
                printf("mycp: cannot overwrite directory '%s' with non-directory\n", source_path);
            }
            else{
                int open_sourcefile  = open(argv[idx], O_RDONLY); 
                if (open_sourcefile  == -1) {
                    printf("mycp: cannot stat '%s': No such file or directory\n", argv[idx]);
                } else {
                    int open_destination_directory = open(source_path, O_WRONLY | O_TRUNC | O_CREAT, source_stat.st_mode); 
                    while ((length = read(open_sourcefile , buf, 8192)) > 0) {
                        if (write(open_destination_directory, buf, length) < length) {
                            break;
                        }
                    }
                    close(open_sourcefile);
                    close(open_destination_directory);
                    memset(source_path, 0, 1024);
                }
            }
        }
    }
}

void CopySourceToDest(int argc, char * argv[], char* last_arg_name, struct stat last_arg_stat) {
    char source_arg[1024];
    ssize_t length;
    struct stat source_stat;

    if (argc > 3) {
        printf("mycp: target '%s' is not a directory\n", last_arg_name);
        exit(1);
    }

    lstat(argv[1], &source_stat);
    if (S_ISDIR(source_stat.st_mode)) {
        printf("mycp: -r not specified; omitting directory '%s'", argv[1]);
        exit(1);
    }

    if (access(argv[1], R_OK) != 0) {
        printf("mycp: cannot open '%s' for reading: Permission denied\n", argv[1]);
        exit(1);
    }

    if (access(last_arg_name, W_OK) != 0) {
        printf("mycp: cannot create regular file '%s': Permission denied\n", argv[2]);
        exit(1);
    }

    if (S_ISLNK(source_stat.st_mode)) {
        readlink(argv[1], source_arg, 1024);
        lstat(source_arg, &source_stat); // lstat을 원본으로 업데이트
    } else {
        strcpy(source_arg, argv[1]);
    }

    int open_sourcefile = open(source_arg, O_RDONLY); 
    if (open_sourcefile < 0) {
        perror(" ");
        exit(1);
    }

    int open_destinationfile = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, source_stat.st_mode); 
    while ((length = read(open_sourcefile, buf, 4096)) > 0) {
        if (write(open_destinationfile, buf, length) < length) {
            break;
        }
    }

    close(open_sourcefile);
    close(open_destinationfile);
    memset(source_arg, 0, 1024);
}