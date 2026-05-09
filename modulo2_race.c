/*
 * Modulo 2: Demostracion de Condicion de Carrera (Race Condition)
 * Universidad de las Americas Puebla (UDLAP)
 * Sistemas Operativos - P26-LIS2062-1
 *
 * Experimento 1: contador SIN proteccion  -> valor corrupto (condicion de carrera)
 * Experimento 2: contador CON mutex       -> valor correcto siempre
 *
 * Compilar: gcc -O0 -o modulo2_race modulo2_race.c -lpthread
 * Ejecutar: ./modulo2_race [N_hilos] [N_iteraciones]
 * Nota: -O0 desactiva optimizaciones para que la carrera sea observable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define LOG_FILE "results/logs/modulo2_race.log"
#define MAX_THREADS 64

/* ── Contadores globales compartidos ── */
static volatile long counter_unsafe = 0;  /* SIN proteccion */
static volatile long counter_safe   = 0;  /* CON mutex      */
static pthread_mutex_t safe_mutex   = PTHREAD_MUTEX_INITIALIZER;

static int  N_threads;
static long N_iters;
static FILE *log_fp;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Hilo INSEGURO: lee-modifica-escribe sin proteccion ── */
static void *increment_unsafe(void *arg) {
    for (long i = 0; i < N_iters; i++) {
        /* Seccion critica SIN MUTEX: race condition aqui */
        long tmp = counter_unsafe;   /* 1. leer   */
        tmp = tmp + 1;               /* 2. sumar  */
        counter_unsafe = tmp;        /* 3. escribir (puede sobrescribirse) */
    }
    return NULL;
}

/* ── Hilo SEGURO: mutex garantiza atomicidad ── */
static void *increment_safe(void *arg) {
    for (long i = 0; i < N_iters; i++) {
        pthread_mutex_lock(&safe_mutex);
        counter_safe++;              /* Operacion atomicamente protegida */
        pthread_mutex_unlock(&safe_mutex);
    }
    return NULL;
}

/* ── Ejecutar experimento y devolver tiempo ── */
static double run_experiment(int safe, long *result) {
    pthread_t tids[MAX_THREADS];
    double t0 = now_sec();
    for (int i = 0; i < N_threads; i++)
        pthread_create(&tids[i], NULL, safe ? increment_safe : increment_unsafe, NULL);
    for (int i = 0; i < N_threads; i++)
        pthread_join(tids[i], NULL);
    double elapsed = now_sec() - t0;
    *result = safe ? counter_safe : counter_unsafe;
    return elapsed;
}

int main(int argc, char *argv[]) {
    N_threads = (argc > 1) ? atoi(argv[1]) : 8;
    N_iters   = (argc > 2) ? atol(argv[2]) : 500000L;

    if (N_threads < 2 || N_threads > MAX_THREADS) { fprintf(stderr, "hilos: 2-%d\n", MAX_THREADS); return 1; }

    log_fp = fopen(LOG_FILE, "w");
    if (!log_fp) { perror("fopen log"); return 1; }

    long expected = (long)N_threads * N_iters;

    printf("\n=== MODULO 2: CONDICION DE CARRERA ===\n");
    printf("    N_hilos=%d  N_iteraciones=%ld  Valor_esperado=%ld\n\n", N_threads, N_iters, expected);
    fprintf(log_fp, "# Modulo2 | threads=%d iters=%ld expected=%ld\n", N_threads, N_iters, expected);

    /* ── Experimento 1: SIN sincronizacion ── */
    printf("--- Experimento 1: SIN mutex (race condition) ---\n");
    long result_unsafe = 0;
    double t_unsafe = run_experiment(0, &result_unsafe);
    long lost_unsafe = expected - result_unsafe;
    double error_pct = (lost_unsafe * 100.0) / expected;

    printf("  Resultado:  %ld\n",  result_unsafe);
    printf("  Esperado:   %ld\n",  expected);
    printf("  Perdidas:   %ld (%.2f%% de escrituras corruptas)\n", lost_unsafe, error_pct);
    printf("  Tiempo:     %.4f s\n\n", t_unsafe);

    fprintf(log_fp, "\n[UNSAFE]\nresult=%ld\nexpected=%ld\nlost=%ld\nerror_pct=%.2f\ntime_s=%.4f\n",
            result_unsafe, expected, lost_unsafe, error_pct, t_unsafe);

    /* ── Experimento 2: CON mutex ── */
    printf("--- Experimento 2: CON mutex (sincronizado) ---\n");
    long result_safe = 0;
    double t_safe = run_experiment(1, &result_safe);
    long lost_safe = expected - result_safe;

    printf("  Resultado:  %ld\n",  result_safe);
    printf("  Esperado:   %ld\n",  expected);
    printf("  Perdidas:   %ld (0.00%% -- consistencia garantizada)\n", lost_safe);
    printf("  Tiempo:     %.4f s\n\n", t_safe);

    fprintf(log_fp, "\n[SAFE]\nresult=%ld\nexpected=%ld\nlost=%ld\nerror_pct=0.00\ntime_s=%.4f\n",
            result_safe, expected, lost_safe, t_safe);

    /* ── Comparacion overhead mutex ── */
    double overhead = ((t_safe - t_unsafe) / t_unsafe) * 100.0;
    printf("=== COMPARACION ===\n");
    printf("  Overhead del mutex:        %.1f%%\n", overhead > 0 ? overhead : 0.0);
    printf("  Escrituras corruptas (sin mutex): %ld de %ld (%.2f%%)\n",
           lost_unsafe, expected, error_pct);
    printf("  Escrituras correctas (con mutex): %ld de %ld (100.00%%)\n",
           result_safe, expected);

    fprintf(log_fp, "\n[COMPARISON]\noverhead_pct=%.1f\nunsafe_error_pct=%.2f\n",
            overhead > 0 ? overhead : 0.0, error_pct);

    /* ── Test de multiples corridas para mostrar no-determinismo ── */
    printf("\n--- Reproducibilidad: 5 corridas sin mutex ---\n");
    fprintf(log_fp, "\n[REPRODUCIBILITY]\nrun,result,lost\n");
    for (int r = 0; r < 5; r++) {
        counter_unsafe = 0;
        long res = 0;
        run_experiment(0, &res);
        printf("  Corrida %d: %ld (perdidas: %ld)\n", r+1, res, expected - res);
        fprintf(log_fp, "%d,%ld,%ld\n", r+1, res, expected - res);
    }
    printf("  (Valores diferentes en cada corrida = no-determinismo confirmado)\n");

    pthread_mutex_destroy(&safe_mutex);
    fclose(log_fp);
    printf("\nLog guardado en: %s\n\n", LOG_FILE);
    return 0;
}
