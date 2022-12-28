#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
double sum = 0.0;
int nthread;
int num;

void* process_partial_sum (void * arg);

int main(int argc, char *argv[]) {
    pthread_t *tid;
    if (argc !=3 ) {
        printf("tsum: missing operand\n[ Usage: tsum NTHREAD NUM ]\n");
        return 0;
    } 
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] < 48 || argv[1][i] > 57) {
            printf("tsum: wrong operand, NTHREAD must be positive integer\n");
            return 0;
        }
    }
    for (int i = 0; argv[2][i]; i++) {
        if (argv[2][i] < 48 || argv[2][i] > 57) {
            printf("tsum: wrong operand, NUM must be positive integer\n");
            return 0;
        }
    }

    nthread = atoi(argv[1]);
    num = atoi(argv[2]); 

    if (1 > nthread || nthread > num) {  // 쓰레드의 수가 1보다 작거나, 계산할 수의 범위보다 크다면
        printf("tsum: wrong operand, NTHREAD must be at least 1 and must be equal to or less than NUM\n");
        return 0;
    }

    tid = (pthread_t*) malloc(sizeof(pthread_t) * nthread);

    for (int idx = 0; idx < nthread; idx++) {
        if ( pthread_create( &tid[idx], NULL, process_partial_sum, (void *)idx) ) {
            printf("Thread creation error!\n"); // 쓰레드 생성 실패에 대한 예외 처리
            return 0;
        }
    }
    for (int idx = 0; idx < nthread; idx++) {
        pthread_join(tid[idx], NULL); 
    }

    pthread_mutex_destroy(&mutex);
    printf("Done. Sum = %f\n", sum);
    pthread_exit(NULL);
}

void *process_partial_sum (void * arg) {
    long id = (long)arg;
    long partial_sum = 0;

    long from = id*(num/nthread)+1;
    long to = (id+1)*(num/nthread);

    if( id+1 == nthread ) {  // 마지막 쓰레드라면
        to += num % nthread;
    }

    for (long value = from; value <= to; value++) {
        partial_sum += value;
    }
    pthread_mutex_lock(&mutex);
    sum += partial_sum;
    pthread_mutex_unlock(&mutex);

    printf("Thread %ld doing part %ld to %ld\n", id, from, to);
    pthread_exit(NULL);
}
