#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void function_fork(void);

int main(void) {
    function_fork();
    return 0;
}

void function_fork(void) {
    int n = 1;

    if (fork() == 0) {
        if (fork() == 0) {
            for (int i = 0; i < 1000; i++) {
                n += 3;
                printf("processo neto, pid=%d, n=%d\n", getpid(), n);
            }
        } else {
            for (int i = 0; i < 1000; i++) {
                n += 2;
                printf("processo filho, pid=%d, n=%d\n", getpid(), n);
            }
            wait(NULL);
        }

    } else {
        
        for (int i = 0; i < 1000; i++) {
            n++;
            printf("processo pai, pid=%d, n=%d\n", getpid(), n);
        }
        wait(NULL);
    }
}