#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define TOTAL_MENSAGENS   32
#define TIPO_DADO         1
#define TIPO_CONFIRMACAO  2

typedef struct {
    long tipo;
    int  valor;
} Mensagem;

static struct timespec instante_inicial;

static double segundos_desde_inicio(void)
{
    struct timespec agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    return (agora.tv_sec - instante_inicial.tv_sec)
         + (agora.tv_nsec - instante_inicial.tv_nsec) / 1e9;
}

static void produtor(int id_fila)
{
    Mensagem dado, confirmacao;

    for (int numero = 1; numero <= TOTAL_MENSAGENS; numero++) {
        dado.tipo  = TIPO_DADO;
        dado.valor = numero;

        printf("[%7.2fs] PRODUTOR   envia %2d e espera confirmacao...\n",
               segundos_desde_inicio(), numero);
        fflush(stdout);

        if (msgsnd(id_fila, &dado, sizeof(int), 0) == -1) {
            perror("produtor: msgsnd"); exit(1);
        }
        // bloqueia ate chegar uma mensagem do tipo CONFIRMACAO
        if (msgrcv(id_fila, &confirmacao, sizeof(int), TIPO_CONFIRMACAO, 0) == -1) {
            perror("produtor: msgrcv"); exit(1);
        }
        printf("[%7.2fs] PRODUTOR   recebeu confirmacao do %2d\n",
               segundos_desde_inicio(), confirmacao.valor);
        fflush(stdout);
    }
    exit(0);
}

static void consumidor(int id_fila, long intervalo_ms)
{
    Mensagem dado, confirmacao;

    for (int contador = 1; contador <= TOTAL_MENSAGENS; contador++) {
        if (msgrcv(id_fila, &dado, sizeof(int), TIPO_DADO, 0) == -1) {
            perror("consumidor: msgrcv"); exit(1);
        }
        printf("[%7.2fs] CONSUMIDOR recebeu %2d, tratando...\n",
               segundos_desde_inicio(), dado.valor);
        fflush(stdout);

        usleep((useconds_t) intervalo_ms * 1000);

        confirmacao.tipo  = TIPO_CONFIRMACAO;
        confirmacao.valor = dado.valor;
        if (msgsnd(id_fila, &confirmacao, sizeof(int), 0) == -1) {
            perror("consumidor: msgsnd"); exit(1);
        }
        printf("[%7.2fs] CONSUMIDOR confirmou %2d\n",
               segundos_desde_inicio(), dado.valor);
        fflush(stdout);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    long intervalo_ms = (argc > 1) ? atol(argv[1]) : 1000;
    if (intervalo_ms <= 0) intervalo_ms = 1000;

    int id_fila = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (id_fila == -1) { perror("msgget"); return 1; }
    printf("Fila criada (id %d)\n", id_fila);
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &instante_inicial);

    pid_t pid_produtor = fork();
    if (pid_produtor == 0) produtor(id_fila);

    pid_t pid_consumidor = fork();
    if (pid_consumidor == 0) consumidor(id_fila, intervalo_ms);

    printf("PIDs: produtor=%d consumidor=%d\n", pid_produtor, pid_consumidor);
    fflush(stdout);

    for (int i = 0; i < 2; i++) wait(NULL);

    msgctl(id_fila, IPC_RMID, NULL);
    printf("Fila removida. Fim.\n");
    return 0;
}
