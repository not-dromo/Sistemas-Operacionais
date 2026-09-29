//Lab 6 - Semáforo

#include <stdio.h>
#include <pthread.h>

#define TAMANHO_VETOR   10000
#define NUMERO_TAREFAS  100
#define VALOR_INICIAL   10

int vetor_global[TAMANHO_VETOR];

// Tarefa n: se n for par soma n, se for impar subtrai n
void *executar_tarefa(void *argumento) {
    int numero_tarefa = (int)(long)argumento;
    int valor = (numero_tarefa % 2 == 0) ? numero_tarefa : -numero_tarefa;

    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        vetor_global[posicao] += valor;
    }

    return NULL;
}

int main(void) {
    // Inicializa o vetor com 10
    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        vetor_global[posicao] = VALOR_INICIAL;
    }

    // Cria as 100 threads (tarefas 1 a 100)
    pthread_t threads[NUMERO_TAREFAS];
    for (int indice = 0; indice < NUMERO_TAREFAS; indice++) {
        pthread_create(&threads[indice], NULL, executar_tarefa, (void *)(long)(indice + 1));
    }

    // Espera todas terminarem
    for (int indice = 0; indice < NUMERO_TAREFAS; indice++) {
        pthread_join(threads[indice], NULL);
    }

    // Valor esperado: 10 + 50 pares de (-impar + par) = 60
    int valor_esperado = VALOR_INICIAL + NUMERO_TAREFAS / 2;
    int posicoes_erradas = 0;
    int primeira_posicao_errada = -1;

    for (int posicao = 0; posicao < TAMANHO_VETOR; posicao++) {
        if (vetor_global[posicao] != valor_esperado) {
            posicoes_erradas++;
            if (primeira_posicao_errada == -1) {
                primeira_posicao_errada = posicao;
            }
        }
    }

    printf("Valor esperado em todas as posicoes: %d\n", valor_esperado);

    if (posicoes_erradas == 0) {
        printf("Resultado: nenhuma posicao errada (nao houve erro de concorrencia nesta execucao).\n");
    } else {
        printf("Resultado: %d posicoes com valor errado (houve erro de concorrencia).\n", posicoes_erradas);
        printf("Exemplo: vetor[%d] = %d\n", primeira_posicao_errada,
               vetor_global[primeira_posicao_errada]);
    }

    return 0;
}