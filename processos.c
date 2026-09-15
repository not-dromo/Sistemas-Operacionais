#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

#define TAM 10000
#define MAX_WORKERS 64

typedef struct {
    int vetor[TAM];
    sem_t inicio;
    struct timespec fim_worker[MAX_WORKERS];
} Compartilhado;

double diferenca_tempo(struct timespec inicio, struct timespec fim) {
    return (fim.tv_sec - inicio.tv_sec) +
           (fim.tv_nsec - inicio.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {

    int num_workers = (argc > 1) ? atoi(argv[1]) : 8;

    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Numero de workers invalido (1-%d)\n", MAX_WORKERS);
        return 1;
    }

    Compartilhado *comp = mmap(
        NULL,
        sizeof(Compartilhado),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (comp == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Inicializa o vetor com 4
    for (int i = 0; i < TAM; i++) {
        comp->vetor[i] = 4;
    }

    // Semáforo compartilhado entre processos
    sem_init(&comp->inicio, 1, 0);

    // Criação dos processos
    for (int w = 0; w < num_workers; w++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {

            // Espera o processo principal liberar
            sem_wait(&comp->inicio);

            // Cada processo percorre TODO o vetor
            for (int i = 0; i < TAM; i++) {
                comp->vetor[i] = comp->vetor[i] * 2 + 2;
            }

            clock_gettime(
                CLOCK_MONOTONIC,
                &comp->fim_worker[w]
            );

            _exit(0);
        }
    }

    // Todos os processos já foram criados.
    // Começa a medição agora.
    struct timespec inicio;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // Libera todos os processos
    for (int w = 0; w < num_workers; w++) {
        sem_post(&comp->inicio);
    }

    // Espera todos terminarem
    for (int w = 0; w < num_workers; w++) {
        wait(NULL);
    }

    // Descobre qual processo terminou por último
    struct timespec ultimo = comp->fim_worker[0];

    for (int w = 1; w < num_workers; w++) {

        if (comp->fim_worker[w].tv_sec > ultimo.tv_sec ||
            (comp->fim_worker[w].tv_sec == ultimo.tv_sec &&
             comp->fim_worker[w].tv_nsec > ultimo.tv_nsec)) {

            ultimo = comp->fim_worker[w];
        }
    }

    double tempo = diferenca_tempo(inicio, ultimo);

    // Verifica automaticamente se todas as posições são iguais
    int iguais = 1;

    for (int i = 1; i < TAM; i++) {
        if (comp->vetor[i] != comp->vetor[0]) {
            iguais = 0;
            break;
        }
    }

    printf(
        "[Processos] workers=%d | todas iguais? %s | valor=%d | tempo=%.6f s\n",
        num_workers,
        iguais ? "SIM" : "NAO",
        comp->vetor[0],
        tempo
    );

    sem_destroy(&comp->inicio);
    munmap(comp, sizeof(Compartilhado));

    return 0;
}