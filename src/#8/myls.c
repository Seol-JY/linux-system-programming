#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <grp.h> 
#include <pwd.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define SHOW_ALL           1
#define SHOW_INODE         2
#define SHOW_SIZE          4
#define SHOW_INDICATOR     8
#define SHOW_LONG         16
#define SHOW_REVERSE      32
#define SHOW_RECURSIVE    64
#define SORT_BY_SIZE     128
#define SORT_BY_TIME     256

char *file_name_token;  // 최종 경로가 디렉터리가 아닌 파일 이름일 경우 처리를 위한 전역변수

unsigned int SetFlag(int argc, char *argv[]);
void ProcessFilename(int argc, char *argv[], unsigned int flag);
void myls(char *dir_path, unsigned int flag);
int IsDir(char *filename);
int IsNotHiddenFile(const struct dirent *dirent_);
int FindOne(const struct dirent *dirent_);
int SortBySize(const struct dirent **dirent_1, const struct dirent **dirent_2);
int SortByTime(const struct dirent **dirent_1, const struct dirent **dirent_2);
void ReverseNameList(struct dirent **name_list, int name_cnt);
void PrintBuffer(struct dirent **name_list, int name_cnt, int total_size, unsigned int flag, char (*name_format)[1024]);
void Formatter(char *dir_path, struct dirent **name_list, int name_cnt, unsigned int flag);
int CountDigit(unsigned long int digit);

int main(int argc, char *argv[]) {
    ProcessFilename(argc, argv, SetFlag(argc, argv));
}

unsigned int SetFlag(int argc, char *argv[]) { // 옵션에 맞는 플래그 세트=
    int opt;
    unsigned int flag = 0;
    while ((opt = getopt(argc, argv, "aisFlrRSt")) != -1) {
        switch (opt) {
        case 'a': flag |= SHOW_ALL;        break;
        case 'i': flag |= SHOW_INODE;      break;
        case 's': flag |= SHOW_SIZE;       break;
        case 'F': flag |= SHOW_INDICATOR;  break;
        case 'l': flag |= SHOW_LONG;       break;
        case 'r': flag |= SHOW_REVERSE;    break;
        case 'R': flag |= SHOW_RECURSIVE;  break;
        case 'S': flag |= SORT_BY_SIZE; flag &= ~SORT_BY_TIME;    break; // 더 뒤에 나온 인자로 세트됨
        case 't': flag |= SORT_BY_TIME; flag &= ~SORT_BY_SIZE;    break;
        case '?':
            puts("Try 'myls --help' for more information.");
            exit(EXIT_FAILURE);
        }
    }
    return flag;
}

void ProcessFilename(int argc, char *argv[], unsigned int flag) {
    if (optind == argc) { // 인자가 없는 경우
        myls(".", flag);
    } else {
        for (int i = optind; i < argc; ++i) {
            if (IsDir(argv[i])) {
                if (argc - optind != 1) { // 인자가 두개 이상인 경우에 출력 하도록
                    printf("%s:\n", argv[i]);
                }
            }
            myls(argv[i], flag);
            if ((argc - i != 1)) { printf("\n"); } // 개행
        }
    }
}

int IsDir(char *file_name) {
    struct stat file_stat;
    stat(file_name, &file_stat);
    return S_ISDIR(file_stat.st_mode);
}

int IsNotHiddenFile(const struct dirent *dirent_) {
    if (strchr(dirent_->d_name, '.') - dirent_->d_name == 0) {
        return 0;
    }
    return 1;
}

int FindOne(const struct dirent *dirent_) {
    return (strcmp(dirent_->d_name, file_name_token) == 0);
}

int SortBySize(const struct dirent **dirent_1, const struct dirent **dirent_2) {
    struct stat stat_1, stat_2;
    lstat((*dirent_1)->d_name, &stat_1);
    lstat((*dirent_2)->d_name, &stat_2);
    if (stat_1.st_size < stat_2.st_size) { return 1; }
    else if (stat_1.st_size > stat_2.st_size) { return -1; }
    return strcoll((*dirent_1)->d_name, (*dirent_2)->d_name);
}

int SortByTime(const struct dirent **dirent_1, const struct dirent **dirent_2) {
    struct stat stat_1, stat_2;
    lstat((*dirent_1)->d_name, &stat_1);
    lstat((*dirent_2)->d_name, &stat_2);
    if (stat_1.st_mtime < stat_2.st_mtime) { return 1; }
    else if (stat_1.st_mtime > stat_2.st_mtime) { return -1; }
    return 0;
}

void ReverseNameList(struct dirent **name_list, int name_cnt) {
  struct dirent *temp;
  for (int i = 0; i < name_cnt / 2; i++) {
    temp = name_list[i];
    name_list[i] = name_list[(name_cnt - 1) - i];
    name_list[(name_cnt - 1) - i] = temp;
  }
}

int CountDigit(unsigned long int digit) {
    if (!digit) {
        return 1;
    }
    int count = 0;
    while(digit != 0) {
        digit /= 10;
        ++count;
    }
    return count;
}

void PrintBuffer(struct dirent **name_list, int name_cnt, int total_size, unsigned int flag, char (*name_format)[1024]) {
    char format_buf[1024]; // 포메팅용 버퍼
    char str_buf[1024]; // 임시 버퍼
    struct winsize winsize_;
    int table_row = 0;
    char (*table)[4096];
    int *max_col;

    // l 옵션
    if ((flag & SHOW_LONG)) {
        printf("total %d\n", total_size); 
        for (int idx = 0; idx<name_cnt; idx++) {
            printf("%s\n", name_format[idx]);
            free(name_list[idx]);
        }
        return;
    } 
    // l옵션 아닐 때 포멧팅
    if (!name_cnt) { return; }
    ioctl(fileno(stdin), TIOCGWINSZ, &winsize_);
    while (1) {
        table_row++;
        int now_max_len = 0;
        for (int i = 0; i<table_row; i++) {
            int tmp = 0;
            for (int j = i; j < name_cnt; j += table_row) {
                tmp += strlen(name_format[j])+2;
            }
            now_max_len = (now_max_len < tmp) ? tmp : now_max_len; 
        }
        if (winsize_.ws_col >= now_max_len ) {
            break;
        }
        if (table_row >= name_cnt/2) {table_row = name_cnt; break;}
    }

    table = (char(*)[4096])malloc(sizeof(char) * 4096 * table_row);
    max_col = (int*)malloc(sizeof(int) * (int)(name_cnt/table_row+1));
    for (int i=0; i<(int)(name_cnt/table_row+1); i++) {  // col 최대길이 배열 초기화
        max_col[i]  = 0;
    }

    for (int i = 0; i < name_cnt; i++) {
        max_col[(int)(i/table_row)] = (max_col[(int)(i/table_row)] < strlen(name_format[i])) ? strlen(name_format[i]) : max_col[(int)(i/table_row)];
    }

    int name_idx = 0;
    while(name_idx < name_cnt){
        for(int i = 0; i < table_row; i++ ) {
            if (name_idx == name_cnt) break;
            if (name_idx < table_row) { strcpy(table[i], ""); }

            sprintf(format_buf, "%%-%ds  ", max_col[(int)(name_idx/table_row)]);
            sprintf(str_buf, format_buf, name_format[name_idx++]);
            strcat(table[i], str_buf);
        }
    }

    if ((flag & SHOW_SIZE)) {
        printf("total %d\n", total_size);
    }
    
    for(int i=0; i<table_row; i++) {
        printf("%s\n", table[i]);
    }

     for (int idx = 0; idx<name_cnt; idx++) { // 동적할당 해제
         free(name_list[idx]);
    }
}

void Formatter(char *dir_path, struct dirent **name_list, int name_cnt, unsigned int flag) {
    int max_name = 0, max_inode = 0, max_block_size = 0, max_nlink = 0, max_size = 0, max_passwd_name = 0, max_group_name = 0;

	char *recursive_path_arr[1024];
    int recursive_cnt = 0;

    int total_size = 0;
    struct stat stat_;
    struct passwd *passwd_;
	struct group *group_;
    unsigned long int *inode = (unsigned long int*)malloc(sizeof(unsigned long int) * name_cnt);
    unsigned long int *block_size = (unsigned long int*)malloc(sizeof(unsigned long int) * name_cnt);
    char (*name_format)[1024] = (char(*)[1024])malloc(sizeof(char) * 1024 * name_cnt);
    unsigned long int *nlink = (unsigned long int*)malloc(sizeof(unsigned long int) * name_cnt);
    unsigned long int *size = (unsigned long int*)malloc(sizeof(unsigned long int) * name_cnt);
    char (*passwd_name)[1024] = (char(*)[1024])malloc(sizeof(char) * 1024 * name_cnt);
    char (*group_name)[1024] = (char(*)[1024])malloc(sizeof(char) * 1024 * name_cnt);

    for (int idx = 0; idx<name_cnt; idx++) { // 정보 모으기
        lstat(name_list[idx]->d_name, &stat_);
        int length;
        if (IsDir(name_list[idx]->d_name)) {
			recursive_path_arr[recursive_cnt++] = name_list[idx]->d_name;
		}
        if (flag & SHOW_INODE) {
            inode[idx] = name_list[idx]->d_ino;
            length = CountDigit(inode[idx]);
            max_inode = (max_inode < length) ? length : max_inode;
        }
        if (flag & SHOW_SIZE) {
            block_size[idx] = (stat_.st_mode &S_IFMT == S_IFLNK) ? (unsigned long int)0 : (unsigned long int)stat_.st_blocks/2;
            length = CountDigit(block_size[idx]);
            max_block_size = (max_block_size < length) ? length : max_block_size;
        }
        if (flag & SHOW_LONG) {
            nlink[idx] = stat_.st_nlink; // nlink
            length = CountDigit(nlink[idx]);
            max_nlink = (max_nlink < length) ? length : max_nlink;

            passwd_ = getpwuid(stat_.st_uid);  // passwd
            strcpy(passwd_name[idx], passwd_->pw_name);
            length = strlen(passwd_name[idx]);
            max_passwd_name = (max_passwd_name < length) ? length : max_passwd_name;

            group_ = getgrgid(stat_.st_gid); // group
			strcpy(group_name[idx], group_->gr_name);
            length = strlen(group_name[idx]);
            max_group_name = (max_group_name < length) ? length : max_group_name;

            size[idx] = stat_.st_size;  // size
            length = CountDigit(size[idx]);
            max_size = (max_size < length) ? length : max_size;
        }
        if ((flag & SHOW_LONG) || (flag & SHOW_SIZE)) {
            total_size += stat_.st_blocks / 2;
        }
    }

    for (int idx = 0; idx<name_cnt; idx++) { // 정보 포멧팅
    
        char str_buf[1024]; // 임시 버퍼
        char format_buf[128]; // 포멧팅용 버퍼
        struct tm *modification_time;
        lstat(name_list[idx]->d_name, &stat_);
        
        sprintf(name_format[idx], "%s", ""); // 출력 버퍼 초기화
        if (flag & SHOW_INODE) {
            sprintf(format_buf, "%%%dlu ", max_inode);
            sprintf(str_buf, format_buf, inode[idx]);
            strcat(name_format[idx], str_buf);
        }
        if (flag & SHOW_SIZE) {
            sprintf(format_buf, "%%%dlu ", max_block_size);
            sprintf(str_buf, format_buf, block_size[idx]);
            strcat(name_format[idx], str_buf);
        }
        if (flag & SHOW_LONG) {
            switch (stat_.st_mode & S_IFMT) {
				case S_IFBLK:  strcat(name_format[idx], "b"); break;
				case S_IFCHR:  strcat(name_format[idx], "c"); break;
				case S_IFDIR:  strcat(name_format[idx], "d"); break;
				case S_IFIFO:  strcat(name_format[idx], "P"); break;
				case S_IFLNK:  strcat(name_format[idx], "l"); break;
				case S_IFREG:  strcat(name_format[idx], "-"); break;
				case S_IFSOCK: strcat(name_format[idx], "s"); break;
				default:                                      break;
			}
            strcat(name_format[idx], stat_.st_mode & S_IRUSR ? "r" : "-"); // 파일 형식
            strcat(name_format[idx], stat_.st_mode & S_IWUSR ? "w" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IXUSR ? "x" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IRGRP ? "r" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IWGRP ? "w" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IXGRP ? "x" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IROTH ? "r" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IWOTH ? "w" : "-");
            strcat(name_format[idx], stat_.st_mode & S_IXOTH ? "x" : "-");
            strcat(name_format[idx], " ");

            sprintf(format_buf, "%%%dlu ", max_nlink);  // nlink
            sprintf(str_buf, format_buf, nlink[idx]);
            strcat(name_format[idx], str_buf);

            sprintf(format_buf, "%%-%ds ", max_passwd_name);  
            sprintf(str_buf, format_buf, passwd_name[idx]);
            strcat(name_format[idx], str_buf);

            sprintf(format_buf, "%%-%ds ", max_group_name);  
            sprintf(str_buf, format_buf, group_name[idx]);
            strcat(name_format[idx], str_buf);
            
            sprintf(format_buf, "%%%dlu ", max_size);  // size
            sprintf(str_buf, format_buf, size[idx]);
            strcat(name_format[idx], str_buf);

            modification_time = localtime(&(stat_.st_mtime)); // modification_time
			strftime(str_buf, sizeof(str_buf), "%b %2d %H:%M ", modification_time);
			strcat(name_format[idx], str_buf);

        }
        strcat(name_format[idx], name_list[idx]->d_name);
        if (flag & SHOW_INDICATOR) {
            char* indicator;
            if (S_ISSOCK(stat_.st_mode)) { indicator = "="; }
            else if (S_ISCHR(stat_.st_mode)) { indicator = ">"; }
			else if (S_ISDIR(stat_.st_mode)) { indicator = "/"; }
			else if (S_ISFIFO(stat_.st_mode)) { indicator = "|"; }
			else if (S_ISLNK(stat_.st_mode)) { indicator = "@"; }
			else if (access(name_list[idx]->d_name, X_OK) == 0) { indicator = "*"; }
			else if (S_ISREG(stat_.st_mode)) { indicator = ""; }

            strcat(name_format[idx], indicator);
        }
    }
    
    PrintBuffer(name_list, name_cnt, total_size, flag, name_format);

    if (flag & SHOW_RECURSIVE) {
        for (int i = 0; i < recursive_cnt; i++) {
			myls(recursive_path_arr[i], flag);
		}
    }

    free(inode);
    free(block_size);
    free(name_format);
    free(nlink);
    free(size);
    free(passwd_name);
    free(group_name);
}

void myls(char *dir_path, unsigned int flag) {
    struct dirent **name_list = NULL;
    int name_cnt = scandir(dir_path,  &name_list, (flag & SHOW_ALL) ? NULL : IsNotHiddenFile, (flag & SORT_BY_SIZE) ? SortBySize : (flag & SORT_BY_TIME) ? SortByTime : alphasort );
    char absolute_path[1024];

    if (name_cnt == -1) { 
        if (errno == 13) { // 권한문제
            printf("myls: cannot open directory '%s': %s\n", dir_path, strerror(errno));
            return;
        } else if (errno == 2) { // 존재하지 않는 파일
            printf("myls: cannot access '%s': %s\n", dir_path, strerror(errno));
            return;
        } else if (errno == 20) { // 디렉터리가 아닌 파일인 경우, 파일과 경로를 분리해 경로를 통해 해당파일 하나만 찾음
            char *token;
            char path[1024] = ".";
            token = strtok(dir_path, "/");
            while (token != NULL) {
                file_name_token = token;
                token = strtok(NULL, "/");
                if (token) { strcat(path, "/"); strcat(path, file_name_token); }
            }
            name_cnt = scandir(path,  &name_list, FindOne, NULL);
        }
    }
    getcwd(absolute_path, 1024);
    chdir(dir_path);
    if (flag & SHOW_REVERSE) { ReverseNameList(name_list, name_cnt); }
    if (flag & SHOW_RECURSIVE){
		printf("\n%s:\n", dir_path);
	}
    Formatter(dir_path, name_list, name_cnt, flag);
    chdir(absolute_path);
    free(name_list);
}
