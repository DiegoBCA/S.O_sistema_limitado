/*
 * Modulo 3: Metricas de CPU - Turnaround, Tiempo de Espera, Throughput
 * Universidad de las Americas Puebla (UDLAP)
 * Sistemas Operativos - P26-LIS2062-1
 *
 * Mide tiempos de turnaround y tiempo de espera bajo distintas cargas.
 * Simula burst de CPU de distintas duraciones y calcula metricas de planificacion.
 *
 * Compilar: gcc -O2 -o modulo3_metrics modulo3_metrics.c -lpthread -lm
 * Ejecutar: ./modulo3_metrics [N_hilos]
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define MAX_TASKS 64
#define LOG_FILE  "results/logs/modulo3_metrics.log"

static FILE *log_fp;

typedef struct {
    int    id;
    int    burst_ms;       /* Duracion de CPU burst */
    double arrival_time;
    double start_time;
    double finish_time;
    double turnaround;     /* finish - arrival */
    double wait_time;      /* start  - arrival */
    double response_time;  /* start  - arrival (mismo en no-preemptivo) */
} Task;

static Task       tasks[MAX_TASKS];
static int        N_tasks;
static sem_t      start_barrier;   /* Barrera de inicio sincronizado */
static double     global_t0;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void *task_thread(void *arg) {
    Task *t = (Task *)arg;

    /* Esperar a que todos los hilos esten listos */
    sem_wait(&start_barrier);

    t->arrival_time = now_sec() - global_t0;
    t->start_time   = t->arrival_time;

    /* Simular CPU burst: bucle de trabajo real para consumir CPU */
    long ops = (long)t->burst_ms * 50000L;
    volatile double acc = 1.0;
    for (long i = 0; i < ops; i++) acc *= 1.0000001;
    (void)acc;

    t->finish_time  = now_sec() - global_t0;
    t->turnaround   = t->finish_time - t->arrival_time;
    t->wait_time    = t->start_time  - t->arrival_time;
    t->response_time = t->wait_time;
    return NULL;
}

static void run_scenario(const char *label, int n, int *bursts) {
    N_tasks = n;
    sem_init(&start_barrier, 0, 0);
    pthread_t tids[MAX_TASKS];

    for (int i = 0; i < n; i++) {
        tasks[i].id       = i + 1;
        tasks[i].burst_ms = bursts[i];
        pthread_create(&tids[i], NULL, task_thread, &tasks[i]);
    }

    global_t0 = now_sec();
    /* Liberar todos simultaneamente */
    for (int i = 0; i < n; i++) sem_post(&start_barrier);
    for (int i = 0; i < n; i++) pthread_join(tids[i], NULL);

    /* Calcular metricas agregadas */
    double sum_tat = 0, sum_wt = 0, sum_rt = 0;
    double max_finish = 0;
    for (int i = 0; i < n; i++) {
        sum_tat += tasks[i].turnaround;
        sum_wt  += tasks[i].wait_time;
        sum_rt  += tasks[i].response_time;
        if (tasks[i].finish_time > max_finish) max_finish = tasks[i].finish_time;
    }

    double throughput = n / max_finish;

    printf("\n--- Escenario: %s (N=%d) ---\n", label, n);
    printf("%-5s %-10s %-12s %-12s %-12s\n", "Tarea", "burst(ms)", "turnaround(s)", "t_espera(s)", "t_respuesta(s)");
    fprintf(log_fp, "\n[%s]\ntask,burst_ms,turnaround_s,wait_s,response_s\n", label);

    for (int i = 0; i < n; i++) {
        printf("T%-4d %-10d %-12.4f %-12.4f %-12.4f\n",
               tasks[i].id, tasks[i].burst_ms,
               tasks[i].turnaround, tasks[i].wait_time, tasks[i].response_time);
        fprintf(log_fp, "T%d,%d,%.4f,%.4f,%.4f\n",
                tasks[i].id, tasks[i].burst_ms,
                tasks[i].turnaround, tasks[i].wait_time, tasks[i].response_time);
    }

    printf("  Avg turnaround:  %.4f s\n", sum_tat / n);
    printf("  Avg t_espera:    %.4f s\n", sum_wt  / n);
    printf("  Throughput:      %.2f tareas/s\n", throughput);
    printf("  Makespan:        %.4f s\n", max_finish);

    fprintf(log_fp, "avg_turnaround=%.4f\navg_wait=%.4f\nthroughput=%.2f\nmakespan=%.4f\n",
            sum_tat/n, sum_wt/n, throughput, max_finish);

    sem_destroy(&start_barrier);
}

int main(int argc, char *argv[]) {
    log_fp = fopen(LOG_FILE, "w");
    if (!log_fp) { perror("fopen"); return 1; }

    printf("\n=== MODULO 3: METRICAS DE CPU Y PLANIFICACION ===\n");
    fprintf(log_fp, "# Modulo3 - Metricas de CPU\n");

    /* Escenario A: cargas homogeneas */
    int bursts_homo[] = {10, 10, 10, 10, 10, 10, 10, 10};
    run_scenario("CARGA_HOMOGENEA_8", 8, bursts_homo);

    /* Escenario B: cargas heterogeneas (simula usuarios con distintas duraciones) */
    int bursts_hetero[] = {5, 20, 2, 30, 8, 15, 1, 25};
    run_scenario("CARGA_HETEROGENEA_8", 8, bursts_hetero);

    /* Escenario C: pocos slots (simula N=2 con 8 usuarios) */
    int bursts_limited[] = {10, 10, 10, 10};
    run_scenario("CARGA_LIMITADA_4", 4, bursts_limited);

    printf("\nLog guardado en: %s\n\n", LOG_FILE);
    fclose(log_fp);
    return 0;
}
