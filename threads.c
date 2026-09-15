#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define TAM 10000
#define MAX_WORKERS 64

int vetor[TAM];

pthread_barrier_t barreira;

struct timespec fim_worker[MAX_WORKERS];

typedef struct {
    int id;
} Args;

double diferenca_tempo(struct timespec inicio, struct timespec fim) {
    return (fim.tv_sec - inicio.tv_sec) +
           (fim.tv_nsec - inicio.tv_nsec) / 1e9;
}

void *trabalhar(void *arg) {

    Args *a = (Args *)arg;

    // Todas as threads esperam umas pelas outras
    pthread_barrier_wait(&barreira);

    // Cada thread percorre TODO o vetor
    for (int i = 0; i < TAM; i++) {
        vetor[i] = vetor[i] * 2 + 2;
    }

    clock_gettime(
        CLOCK_MONOTONIC,
        &fim_worker[a->id]
    );

    return NULL;
}

int main(int argc, char *argv[]) {

    int num_workers = (argc > 1) ? atoi(argv[1]) : 8;

    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Numero de workers invalido (1-%d)\n", MAX_WORKERS);
        return 1;
    }

    // Inicializa o vetor
    for (int i = 0; i < TAM; i++) {
        vetor[i] = 4;
    }

    pthread_t threads[MAX_WORKERS];
    Args args[MAX_WORKERS];

    /*
     * +1 porque o processo principal também participa
     * da barreira.
     */
    pthread_barrier_init(
        &barreira,
        NULL,
        num_workers + 1
    );

    // Criação das threads
    for (int w = 0; w < num_workers; w++) {

        args[w].id = w;

        pthread_create(
            &threads[w],
            NULL,
            trabalhar,
            &args[w]
        );
    }

    /*
     * Todas as threads já foram criadas.
     * O processo principal entra na barreira.
     */
    pthread_barrier_wait(&barreira);

    /*
     * A partir daqui as threads começam a trabalhar.
     */
    struct timespec inicio;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // Espera todas as threads terminarem
    for (int w = 0; w < num_workers; w++) {
        pthread_join(threads[w], NULL);
    }

    // Descobre qual thread terminou por último
    struct timespec ultimo = fim_worker[0];

    for (int w = 1; w < num_workers; w++) {

        if (fim_worker[w].tv_sec > ultimo.tv_sec ||
            (fim_worker[w].tv_sec == ultimo.tv_sec &&
             fim_worker[w].tv_nsec > ultimo.tv_nsec)) {

            ultimo = fim_worker[w];
        }
    }

    double tempo = diferenca_tempo(inicio, ultimo);

    // Verifica automaticamente se todas as posições são iguais
    int iguais = 1;

    for (int i = 1; i < TAM; i++) {
        if (vetor[i] != vetor[0]) {
            iguais = 0;
            break;
        }
    }

    printf(
        "[Threads] workers=%d | todas iguais? %s | valor=%d | tempo=%.6f s\n",
        num_workers,
        iguais ? "SIM" : "NAO",
        vetor[0],
        tempo
    );

    pthread_barrier_destroy(&barreira);

    return 0;
}