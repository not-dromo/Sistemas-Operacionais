/*
 * LAB7 - Parte 1: troca de mensagens ASSINCRONA com fila do sistema (sys/msg.h)
 *
 * 3 processos: 1 produtor e 2 consumidores.
 *  - produtor: gera 1 inteiro por segundo (256 no total)
 *  - consumidor 1: retira 1 mensagem a cada 1 segundo
 *  - consumidor 2: retira 1 mensagem a cada 2 segundos
 *  - a fila guarda no maximo 64 elementos
 *
 * Fim do programa: depois de enviar as 256 mensagens, o produtor envia
 * 2 mensagens especiais (valor = -1), uma para cada consumidor. Quem
 * recebe -1 para de consumir. Como a fila e FIFO, as mensagens especiais
 * so sao lidas depois de todos os dados.
 *
 * Uso: ./lab7_assincrono [intervalo_base_em_ms]   (padrao: 1000)
 * O argumento serve so para testar rapido (ex.: 10).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define TOTAL_MENSAGENS      256
#define TAMANHO_MAXIMO_FILA  64
#define VALOR_FIM            -1
#define TIPO_MENSAGEM        1

typedef struct {
    long tipo;   /* obrigatorio: precisa ser > 0 */
    int  valor;  /* o inteiro que estamos enviando */
} Mensagem;

static struct timespec instante_inicial;

static double segundos_desde_inicio(void)
{
    struct timespec agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    return (agora.tv_sec - instante_inicial.tv_sec)
         + (agora.tv_nsec - instante_inicial.tv_nsec) / 1e9;
}

static long mensagens_na_fila(int id_fila)
{
    struct msqid_ds info;
    if (msgctl(id_fila, IPC_STAT, &info) == -1) return -1;
    return (long) info.msg_qnum;
}

static void esperar_ms(long milissegundos)
{
    usleep((useconds_t) milissegundos * 1000);
}

static void produtor(int id_fila, long intervalo_ms)
{
    Mensagem mensagem;
    mensagem.tipo = TIPO_MENSAGEM;

    for (int numero = 1; numero <= TOTAL_MENSAGENS; numero++) {
        mensagem.valor = numero;
        /* sem IPC_NOWAIT: se a fila estiver cheia (64), bloqueia aqui */
        if (msgsnd(id_fila, &mensagem, sizeof(int), 0) == -1) {
            perror("produtor: msgsnd");
            exit(1);
        }
        printf("[%7.2fs] PRODUTOR    enviou %3d  | fila: %2ld/%d\n",
               segundos_desde_inicio(), numero,
               mensagens_na_fila(id_fila), TAMANHO_MAXIMO_FILA);
        fflush(stdout);
        esperar_ms(intervalo_ms);
    }

    /* uma mensagem de fim para cada consumidor */
    mensagem.valor = VALOR_FIM;
    for (int consumidor = 0; consumidor < 2; consumidor++) {
        if (msgsnd(id_fila, &mensagem, sizeof(int), 0) == -1) {
            perror("produtor: msgsnd (fim)");
            exit(1);
        }
    }
    printf("[%7.2fs] PRODUTOR    terminou: %d mensagens enviadas\n",
           segundos_desde_inicio(), TOTAL_MENSAGENS);
    fflush(stdout);
    exit(0);
}

static void consumidor(int id_fila, int numero_consumidor, long intervalo_ms)
{
    Mensagem mensagem;
    int quantidade_consumida = 0;

    while (1) {
        /* tipo 0 = pega a primeira mensagem da fila; bloqueia se vazia */
        if (msgrcv(id_fila, &mensagem, sizeof(int), 0, 0) == -1) {
            perror("consumidor: msgrcv");
            exit(1);
        }
        if (mensagem.valor == VALOR_FIM) break;

        quantidade_consumida++;
        printf("[%7.2fs] CONSUMIDOR %d recebeu %3d  | fila: %2ld/%d\n",
               segundos_desde_inicio(), numero_consumidor, mensagem.valor,
               mensagens_na_fila(id_fila), TAMANHO_MAXIMO_FILA);
        fflush(stdout);
        esperar_ms(intervalo_ms);
    }

    printf("[%7.2fs] CONSUMIDOR %d terminou: consumiu %d mensagens\n",
           segundos_desde_inicio(), numero_consumidor, quantidade_consumida);
    fflush(stdout);
    exit(0);
}

int main(int argc, char *argv[])
{
    long intervalo_base_ms = (argc > 1) ? atol(argv[1]) : 1000;
    if (intervalo_base_ms <= 0) intervalo_base_ms = 1000;

    /* cria a fila (privada, so os filhos desse programa usam) */
    int id_fila = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (id_fila == -1) { perror("msgget"); return 1; }

    /* limita a fila: msg_qbytes conta so os bytes dos dados (int = 4 bytes) */
    struct msqid_ds info;
    if (msgctl(id_fila, IPC_STAT, &info) == -1) { perror("msgctl IPC_STAT"); return 1; }
    info.msg_qbytes = TAMANHO_MAXIMO_FILA * sizeof(int);
    if (msgctl(id_fila, IPC_SET, &info) == -1) { perror("msgctl IPC_SET"); return 1; }

    printf("Fila criada (id %d), capacidade: %d mensagens (%lu bytes)\n",
           id_fila, TAMANHO_MAXIMO_FILA, (unsigned long) info.msg_qbytes);
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &instante_inicial);

    pid_t pid_produtor = fork();
    if (pid_produtor == 0) produtor(id_fila, intervalo_base_ms);

    pid_t pid_consumidor1 = fork();
    if (pid_consumidor1 == 0) consumidor(id_fila, 1, intervalo_base_ms);

    pid_t pid_consumidor2 = fork();
    if (pid_consumidor2 == 0) consumidor(id_fila, 2, 2 * intervalo_base_ms);

    printf("PIDs: produtor=%d consumidor1=%d consumidor2=%d\n",
           pid_produtor, pid_consumidor1, pid_consumidor2);
    fflush(stdout);

    for (int i = 0; i < 3; i++) wait(NULL);

    msgctl(id_fila, IPC_RMID, NULL);   /* apaga a fila do sistema */
    printf("Fila removida. Fim.\n");
    return 0;
}
