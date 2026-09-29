//Lab 6 - Semáforo

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define TAMANHO_VETOR               10000
#define VALOR_INICIAL               10
#define MAXIMO_POSICOES_IMPRESSAS   20

int vetor_global[TAMANHO_VETOR];
sem_t semaforos[TAMANHO_VETOR];   // um semaforo por posicao
int usar_semaforos = 0;

// Tarefa n: se n for par soma n, se for impar subtrai n
int valor_da_tarefa(int numero_tarefa) {
    return (numero_tarefa % 2 == 0) ? numero_tarefa : -numero_tarefa;
}

void *executar_tarefa(void *argumento) {
    int valor = valor_da_tarefa((int)(long)argumento);

    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        if (usar_semaforos) {
            sem_wait(&semaforos[posicao]);   // Down
        }

        vetor_global[posicao] += valor;

        if (usar_semaforos) {
            sem_post(&semaforos[posicao]);   // Up
        }
    }

    return NULL;
}

void executar_rodada(int numero_tarefas, int com_semaforos) {
    usar_semaforos = com_semaforos;

    // Reinicia o vetor
    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        vetor_global[posicao] = VALOR_INICIAL;
    }

    pthread_t *threads = malloc(numero_tarefas * sizeof(pthread_t));

    for (int indice = 0; indice < numero_tarefas; indice++) {
        pthread_create(&threads[indice], NULL, executar_tarefa, (void *)(long)(indice + 1));
    }
    for (int indice = 0; indice < numero_tarefas; indice++) {
        pthread_join(threads[indice], NULL);
    }
    free(threads);

    // Valor esperado = valor inicial + soma de todas as tarefas
    int valor_esperado = VALOR_INICIAL;
    for (int numero_tarefa = 1; numero_tarefa <= numero_tarefas; numero_tarefa++) {
        valor_esperado += valor_da_tarefa(numero_tarefa);
    }

    // Arquivo com todas as posicoes erradas (util para o relatorio)
    FILE *arquivo = NULL;
    if (!com_semaforos) {
        arquivo = fopen("posicoes_erradas.txt", "w");
    }

    int posicoes_erradas = 0;

    printf("\n=== %s | %d tarefas ===\n",
           com_semaforos ? "COM semaforos" : "SEM semaforos", numero_tarefas);
    printf("Valor esperado em todas as posicoes: %d\n", valor_esperado);

    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        if (vetor_global[posicao] != valor_esperado) {
            if (posicoes_erradas < MAXIMO_POSICOES_IMPRESSAS) {
                printf("  vetor[%d] = %d\n", posicao, vetor_global[posicao]);
            }
            if (arquivo != NULL) {
                fprintf(arquivo, "vetor[%d] = %d\n", posicao, vetor_global[posicao]);
            }
            posicoes_erradas++;
        }
    }

    if (posicoes_erradas == 0) {
        printf("Nenhuma posicao errada.\n");
    } else {
        printf("Total de posicoes erradas: %d de %d\n", posicoes_erradas, TAMANHO_VETOR);
        if (posicoes_erradas > MAXIMO_POSICOES_IMPRESSAS) {
            printf("(mostrando apenas as %d)\n", MAXIMO_POSICOES_IMPRESSAS);
        }
    }

    if (arquivo != NULL) {
        fclose(arquivo);
    }
}

int main(int argc, char *argv[]) {
    int numero_tarefas = 100;
    if (argc > 1) {
        numero_tarefas = atoi(argv[1]);
    }

    // Semaforos binarios (valor inicial 1), compartilhados entre threads (pshared = 0)
    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        sem_init(&semaforos[posicao], 0, 1);
    }

    executar_rodada(numero_tarefas, 0);   // sem protecao
    executar_rodada(numero_tarefas, 1);   // com semaforos

    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        sem_destroy(&semaforos[posicao]);
    }

    return 0;
}

// Compilar: gcc com_semaforo.c -o com_semaforo -pthread