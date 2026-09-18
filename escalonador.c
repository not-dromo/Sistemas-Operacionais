//Lab 4 - Escalonador Round-Robin com Preempcao

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_PROCESSOS 6
#define QUANTUM_SEGUNDOS 1
#define MAX_SLICES 5

// Codigo executado por cada processo filho: um loop eterno de trabalho.
void codigo_processo(int id) {
    long long contador = 0;
    while (1) {
        contador++;
        if (contador % 100000000LL == 0) {
            printf("    [Processo %d | PID %d] ainda executando... contador=%lld\n", id, getpid(), contador);
            fflush(stdout);
        }
    }
}

int main(void) {
    pid_t pids[NUM_PROCESSOS];
    int slices_executados[NUM_PROCESSOS] = {0};
    int encerrado[NUM_PROCESSOS] = {0};

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[Escalonador] Criando %d processos...\n", NUM_PROCESSOS);

    for (int i = 0; i < NUM_PROCESSOS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            raise(SIGSTOP);
            codigo_processo(i);
            exit(0);
        } else {
            pids[i] = pid;
        }
    }

    for (int i = 0; i < NUM_PROCESSOS; i++) {
        int status;
        waitpid(pids[i], &status, WUNTRACED);
    }

    printf("[Escalonador] Todos os processos criados e pausados.\n");
    printf("[Escalonador] Iniciando Round-Robin (quantum = %ds, limite = %d slices)\n\n",
           QUANTUM_SEGUNDOS, MAX_SLICES);

    int processos_ativos = NUM_PROCESSOS;
    int atual = 0;

    while (processos_ativos > 0) {
        if (encerrado[atual]) {
            atual = (atual + 1) % NUM_PROCESSOS;
            continue;
        }

        printf("[Escalonador] -> Processo %d (PID %d) recebe a CPU "
               "(slice %d/%d)\n",
               atual, pids[atual], slices_executados[atual] + 1, MAX_SLICES);
        kill(pids[atual], SIGCONT);

        sleep(QUANTUM_SEGUNDOS);

        kill(pids[atual], SIGSTOP);
        int status;
        waitpid(pids[atual], &status, WUNTRACED);

        slices_executados[atual]++;
        printf("[Escalonador] <- Quantum esgotado. Processo %d preemptado "
               "(slice %d/%d concluido)\n",
               atual, slices_executados[atual], MAX_SLICES);

        if (slices_executados[atual] >= MAX_SLICES) {
            printf("[Escalonador] Processo %d atingiu o limite de %d "
                   "time-slices. Encerrando (SIGKILL)...\n\n",
                   atual, MAX_SLICES);
            kill(pids[atual], SIGKILL);
            waitpid(pids[atual], &status, 0);
            encerrado[atual] = 1;
            processos_ativos--;
        } else {
            printf("\n");
        }

        atual = (atual + 1) % NUM_PROCESSOS;
    }

    printf("[Escalonador] Todos os %d processos foram encerrados. "
           "Fim da execucao.\n", NUM_PROCESSOS);

    return 0;
}