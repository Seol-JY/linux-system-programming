#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_HISTORY_SIZE 5 // 저장할 최대 History 개수

char history_queue[MAX_HISTORY_SIZE][1024]; 
int queue_start_idx = 0; // queue가 시작되는 인덱스

typedef struct CMD {  // 명령에 대한 정보를 담는 구조체
    char* name; // 명령이름
    char* desc; // 명령설명
    int (*cmd)(int argc, char* argv[]); // 명령 실행 함수에 대한 포인터
} CMD;

void loadHistory();
void storeHistory();

int cmd_cd(int argc, char* argv[]);
int cmd_pwd(int argc, char* argv[]);
int cmd_exit(int argc, char* argv[]);
int cmd_help(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_history(int argc, char* argv[]);

int cmdProcessing(void);

CMD builtin[] = {
    {"cd","작업 디렉터리 바꾸기", cmd_cd},
    {"pwd","현재 작업 디렉터리는?", cmd_pwd},
    {"exit","셸 실행을 종료합니다", cmd_exit},
    {"help","도움말 보여 주기", cmd_help},
    {"echo","입력된 텍스트를 표시", cmd_echo},
    {"history","명령어 기록을 표시", cmd_history}
};

const int builtins = sizeof(builtin) / sizeof(CMD); // 내장 명령의 수

int main(void) {

    loadHistory(); // 기록을 불러옴
    int isExit = 0;
    while (!isExit)
        isExit = cmdProcessing(); // 명령 처리 루틴 호출
    fputs("My Shell을 종료합니다...\n", stdout);
    storeHistory(); // 기록을 저장함
    return 0;
}

#define STR_LEN 1024  // 최대 명령 입력 길이
#define MAX_TOKENS 128  // 공백으로 분리되는 명령어 문자열 최대 수

int cmdProcessing(void) {
    char cmdLine[STR_LEN]; // 입력 명령 전체를 저장하는 배열
    char* cmdTokens[MAX_TOKENS]; // 입력 명령을 공백으로 분리하여 저장하는 배열
    char delim[] = "\t\n\r "; // 토큰 구분자 = strtok에서 사용
    char* token; // 하나의 토큰을 분리하는데 사용
    int tokenNum; //입력 명령에 저장된 토큰 수
    
    /*기타 필요한 변수*/
    
    int exitcode = 0; // 종료 코드(default = 0)
    fputs("[mysh v0.1] $ ", stdout); //프롬프트 출력
    fgets(cmdLine, STR_LEN, stdin); // 한 줄의 명령 입력

    strcpy(history_queue[(queue_start_idx++)%MAX_HISTORY_SIZE], (const char *)cmdLine); // 빈 문자열이 아니면 저장

    tokenNum = 0;
    token = strtok(cmdLine, delim); // 입력 명령의 문자열 하나 분리

    while (token) { // 문자열이 있을 동안 반복
        cmdTokens[tokenNum++] = token; // 분리된 문자열을 배열에 저장
        token = strtok(NULL, delim); // 연속하여 입력 명령의 문자열 하나 분리
    }
    
    cmdTokens[tokenNum] = NULL;
    
    if (tokenNum == 0)
        return exitcode;
    for (int i = 0; i < builtins; ++i) { // 내장 명령인지 검사하여 명령이 있으면 해당 함수 호출
        if(strcmp(cmdTokens[0], builtin[i].name) == 0)
        return builtin[i].cmd(tokenNum, cmdTokens);
    }
    /*내장 명령이 아닌 경우 자식 프로세스를 생성하여 명령 프로그램을 호출함*/
    char path_buf[1024];
    int status;
    pid_t pid;
    pid = fork();

    if(pid == 0) {
        sprintf(path_buf,"/bin/%s", cmdTokens[0]);
        if (execv(path_buf, cmdTokens) == -1) {
            printf("%s: 명령을 찾을 수 없습니다\n", cmdTokens[0]);
            exit(1);
        };
    } else if(pid > 0) {
        waitpid(pid, &status, 0);
    } else {
        printf("프로그램 실행에 실패했습니다.");
    }
    return exitcode;
}

int cmd_cd(int argc, char* argv[]) {
    struct stat stat_;
    char path_buf[1024];

    if (argc > 2) {
        printf("-mysh: cd: 인자가 너무 많음\n");
    }
    else if (argc == 1 || (strcmp(argv[1], "~") == 0)) {
        chdir(getenv("HOME")); // 실제로 기능 수행
    }
    else if (argc == 2) {
        if      (stat(argv[1], &stat_) == -1) { printf("-mysh: cd: %s: 그런 파일이나 디렉터리가 없습니다\n", argv[1]); }   
        else if (!S_ISDIR(stat_.st_mode))     { printf("-mysh: cd: %s: 디렉터리가 아닙니다\n", argv[1]); }
        else if (access(argv[1], X_OK) == -1) { printf("-mysh: cd: %s: 허가 거부\n", argv[1]); }
        else                                  { chdir(argv[1]); } // 실제로 기능 수행
    }
    return 0;
}

int cmd_pwd(int argc, char* argv[]) {
    char buf [1024];
    if (getcwd(buf, 1024) == NULL) {
        printf("-mysh: pwd: 오류 발생\n");
        return 0;
    }
    printf("%s\n", buf);
    return 0;
}

int cmd_exit(int argc, char* argv[]) { return 1; }

int cmd_help(int argc, char* argv[]) {
    if (argc == 1) {
        for (int idx = 0; idx < builtins; idx++) {
            printf("%-10s: %s\n", builtin[idx].name, builtin[idx].desc);
        }
    } else {
        for (int argv_idx = 1; argv_idx < argc; argv_idx++) {
            for (int idx = 0; idx < builtins; idx++) {
                if (!strcmp(argv[argv_idx], builtin[idx].name)) {
                    printf("%-10s: %s\n", builtin[idx].name, builtin[idx].desc);
                    break;
                } else if (idx == builtins -1) {
                    printf("-mysh: help: '%s'에 해당하는 도움말 주제가 없습니다. 'man -k %s' 또는 'info %s'를 사용해 보세요.\n", argv[argv_idx], argv[argv_idx], argv[argv_idx]);
                }
            }
        }
    }
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    if (argc == 1) {  putchar(' '); }
    for (int idx = 1; idx < argc; idx++) { 
		fputs(argv[idx],stdout);
		if (idx != argc - 1) putchar(' ');
    }
    putchar('\n');

    return 0;
}

int cmd_history(int argc, char* argv[]) {
    int idx = queue_start_idx;
    while (!strcmp(history_queue[(idx)%MAX_HISTORY_SIZE], "")) { (idx++) % MAX_HISTORY_SIZE;} // MAX_SIZE만큼 차지 않은 경우 인덱스 처리
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        if (!strcmp(history_queue[(idx)%MAX_HISTORY_SIZE], "")) break;
        printf("%5d  %s", i+1, history_queue[(idx++)%MAX_HISTORY_SIZE]);
        if (!strcmp(history_queue[(idx-1)%MAX_HISTORY_SIZE], "")) printf("\n");
    } 
    return 0;
}

void loadHistory() {
    // 기록을 불러오는 함수
    char path_buf[1024];
    char line_buf[1024];
    sprintf(path_buf, "%s/%s", getenv("HOME"), ".mysh_history");
    FILE *mysh_history = fopen(path_buf, "r");

    if (mysh_history) { 
        while ((fgets(line_buf, 1024, mysh_history))) {
            strcpy(history_queue[(queue_start_idx++)%MAX_HISTORY_SIZE], line_buf);
        }
        fclose(mysh_history);
    }
}

void storeHistory() {
    // 기록을 저장하는 함수
    char path_buf[1024];
    char line_buf[1024];
    sprintf(path_buf, "%s/%s", getenv("HOME"), ".mysh_history");
    FILE *mysh_history = fopen(path_buf, "w");

    if (!mysh_history) { printf("History를 쓸 수 없습니다."); exit(1); }
  
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {   
        if (history_queue[queue_start_idx%MAX_HISTORY_SIZE] != NULL) {  // 빈문자열이 아니면 파일에 씀
            fputs(history_queue[(queue_start_idx++)%MAX_HISTORY_SIZE], mysh_history);
        } else {queue_start_idx++;}
    }
    fclose(mysh_history);
}


