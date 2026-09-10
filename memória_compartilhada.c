#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>

void function_fork(int *n);

int main(void) {
    // cria o bloco de memória compartilhada
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    // anexa na memória do processo
    int *n = (int *) shmat(shmid, NULL, 0);
    if (n == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    *n = 1;

    function_fork(n);

    // pai limpa o bloco de memória compartilhada por último, depois de tudo terminar
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

void function_fork(int *n) {
    if (fork() == 0) {
        if (fork() == 0) {
            // neto
            for (int i = 0; i < 1000; i++) {
                *n += 3;
                printf("processo neto, pid=%d, n=%d\n", getpid(), *n);
            }
            shmdt(n);
            exit(0);
        } else {
            // filho
            for (int i = 0; i < 1000; i++) {
                *n += 2;
                printf("processo filho, pid=%d, n=%d\n", getpid(), *n);
            }
            wait(NULL); // espera o neto
            shmdt(n);
            exit(0);
        }
    } else {
        // pai
        for (int i = 0; i < 1000; i++) {
            *n += 1;
            printf("processo pai, pid=%d, n=%d\n", getpid(), *n);
        }
        wait(NULL); // espera o filho
        shmdt(n);
    }
}