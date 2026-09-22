//Lab 5 - Escalonamento de processos de tempo real

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

void processo_filho(const char *nome) {
    while (1) {
        printf("[%s] executando (pid=%d)\n", nome, getpid());
        fflush(stdout);
        sleep(1);
    }
}

int main(void) {
    pid_t pid_p1, pid_p2, pid_p3;

    // Cria os tres filhos
    pid_p1 = fork();
    if (pid_p1 == 0) {
        processo_filho("P1");
        exit(0);
    }

    pid_p2 = fork();
    if (pid_p2 == 0) {
        processo_filho("P2");
        exit(0);
    }

    pid_p3 = fork();
    if (pid_p3 == 0) {
        processo_filho("P3");
        exit(0);
    }

    // A partir daqui so o pai (escalonador) continua executando este codigo

    kill(pid_p1, SIGSTOP);
    kill(pid_p2, SIGSTOP);
    kill(pid_p3, SIGSTOP);

    int p1_esta_rodando = 0;
    int p2_esta_rodando = 0;
    int p3_esta_rodando = 0;

    struct timeval agora;

    while (1) {
        gettimeofday(&agora, NULL);

        int segundo_do_minuto = agora.tv_sec % 60;

        int p1_deve_rodar = (segundo_do_minuto >= 5  && segundo_do_minuto < 25);
        int p2_deve_rodar = (segundo_do_minuto >= 45 && segundo_do_minuto < 60);
        int p3_deve_rodar = !p1_deve_rodar && !p2_deve_rodar;

        if (p1_deve_rodar && !p1_esta_rodando) {
            kill(pid_p1, SIGCONT);
            p1_esta_rodando = 1;
        } else if (!p1_deve_rodar && p1_esta_rodando) {
            kill(pid_p1, SIGSTOP);
            p1_esta_rodando = 0;
        }

        if (p2_deve_rodar && !p2_esta_rodando) {
            kill(pid_p2, SIGCONT);
            p2_esta_rodando = 1;
        } else if (!p2_deve_rodar && p2_esta_rodando) {
            kill(pid_p2, SIGSTOP);
            p2_esta_rodando = 0;
        }

        if (p3_deve_rodar && !p3_esta_rodando) {
            kill(pid_p3, SIGCONT);
            p3_esta_rodando = 1;
        } else if (!p3_deve_rodar && p3_esta_rodando) {
            kill(pid_p3, SIGSTOP);
            p3_esta_rodando = 0;
        }

        usleep(100000);
    }

    return 0;
}