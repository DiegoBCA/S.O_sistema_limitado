/*
 * Modulo 1: Sistema de Acceso Limitado a Pagina Web
 * Universidad de las Americas Puebla (UDLAP)
 * Sistemas Operativos - P26-LIS2062-1
 *
 * Demuestra: semaforo contador + mutex para sincronizacion
 * de N hilos concurrentes con acceso limitado.
 *
 * Compilar: gcc -o modulo1_acceso modulo1_acceso.c -lpthread -lrt
 * Ejecutar: ./modulo1_acceso [N_slots] [N_usuarios] [seed]
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define MAX_USERS 32
#define LOG_FILE  "results/logs/modulo1_access.log"

/* ── Recursos compartidos ── */
static int         active_count  = 0;  /* Seccion critica: mutex protege esto */
static int         total_waiting = 0;
static int         completed     = 0;
static sem_t       slots;              /* Semaforo contador = capacidad N */
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE       *log_fp;

/* ── Estructura de cada hilo-usuario ── */
typedef struct {
    int    id;
    double wait_time;     /* Tiempo esperando en cola (sem_wait) */
    double browse_time;   /* Tiempo navegando dentro del sistema */
    double enter_ts;      /* Timestamp de entrada a pagina */
    int    was_queued;    /* 1 si tuvo que esperar en cola */
} UserThread;

static UserThread users[MAX_USERS];
static int        N_slots;
static int        N_users;

/* ── Utilidades de tiempo ── */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void ts_str(char *buf, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);
    strftime(buf, len, "%H:%M:%S", tm_info);
    snprintf(buf + 8, len - 8, ".%03ld", ts.tv_nsec / 1000000);
}

static void log_event(const char *event, int uid, int active, int free_slots) {
    char tbuf[32];
    ts_str(tbuf, sizeof(tbuf));
    fprintf(log_fp, "[%s] %s | user=T%02d | active=%d | free_slots=%d\n",
            tbuf, event, uid, active, free_slots);
    fflush(log_fp);
    printf("[%s] %-30s | T%02d | activos=%d | slots_libres=%d\n",
           tbuf, event, uid, active, free_slots);
}

/* ── Funcion del hilo ── */
static void *user_thread(void *arg) {
    UserThread *u = (UserThread *)arg;
    int uid = u->id;

    double t0 = now_sec();

    /* ── Ciclo de vida: SOLICITAR ── */
    int sem_val;
    sem_getvalue(&slots, &sem_val);
    if (sem_val == 0) {
        u->was_queued = 1;
        pthread_mutex_lock(&db_mutex);
        total_waiting++;
        log_event("COLA_FIFO_ESPERA  ", uid, active_count, sem_val);
        pthread_mutex_unlock(&db_mutex);
    }

    /* sem_wait: bloquea si slots == 0 (FIFO garantizado por Linux NPTL) */
    sem_wait(&slots);

    double t_enter = now_sec();
    u->wait_time = t_enter - t0;

    /* ── Seccion critica: CHECK-IN (atomico) ── */
    pthread_mutex_lock(&db_mutex);
    active_count++;
    total_waiting = (total_waiting > 0) ? total_waiting - 1 : 0;
    int cur_free;
    sem_getvalue(&slots, &cur_free);
    log_event("ACCESO_CONCEDIDO  ", uid, active_count, cur_free);
    pthread_mutex_unlock(&db_mutex);

    /* ── NAVEGACION: mutex libre para otros hilos ── */
    u->enter_ts = t_enter;
    int browse_ms = 500 + rand() % 2000;  /* 0.5 - 2.5 s */
    usleep(browse_ms * 1000);

    /* ── Seccion critica: CHECK-OUT (atomico) ── */
    pthread_mutex_lock(&db_mutex);
    active_count--;
    completed++;
    sem_getvalue(&slots, &cur_free);
    log_event("PAGINA_LIBERADA   ", uid, active_count, cur_free + 1);
    pthread_mutex_unlock(&db_mutex);

    /* Liberar semaforo DESPUES del mutex (jerarquia anti-deadlock) */
    sem_post(&slots);

    u->browse_time = now_sec() - t_enter;
    return NULL;
}

/* ── Main ── */
int main(int argc, char *argv[]) {
    N_slots = (argc > 1) ? atoi(argv[1]) : 3;
    N_users = (argc > 2) ? atoi(argv[2]) : 8;
    int seed = (argc > 3) ? atoi(argv[3]) : 42;

    if (N_slots < 1 || N_slots > MAX_USERS) { fprintf(stderr, "N_slots: 1-%d\n", MAX_USERS); return 1; }
    if (N_users < 1 || N_users > MAX_USERS) { fprintf(stderr, "N_users: 1-%d\n", MAX_USERS); return 1; }

    srand(seed);

    log_fp = fopen(LOG_FILE, "w");
    if (!log_fp) { perror("fopen log"); return 1; }

    printf("\n=== MODULO 1: ACCESO LIMITADO A PAGINA WEB ===\n");
    printf("    N_slots=%d  N_usuarios=%d  seed=%d\n\n", N_slots, N_users, seed);
    fprintf(log_fp, "# Modulo1 | N_slots=%d N_users=%d seed=%d\n", N_slots, N_users, seed);

    /* Inicializar semaforo con capacidad N */
    if (sem_init(&slots, 0, N_slots) != 0) { perror("sem_init"); return 1; }

    pthread_t tids[MAX_USERS];
    double t_global_start = now_sec();

    /* Crear hilos con pequeno escalonamiento para simular llegadas reales */
    for (int i = 0; i < N_users; i++) {
        users[i].id         = i + 1;
        users[i].wait_time  = 0;
        users[i].browse_time = 0;
        users[i].was_queued = 0;
        pthread_create(&tids[i], NULL, user_thread, &users[i]);
        usleep((rand() % 300) * 1000);  /* 0-300ms entre llegadas */
    }

    /* Esperar todos los hilos */
    for (int i = 0; i < N_users; i++) pthread_join(tids[i], NULL);

    double total_elapsed = now_sec() - t_global_start;

    /* ── Resultados y metricas ── */
    printf("\n=== RESULTADOS ===\n");
    printf("%-6s %-12s %-14s %-10s\n", "Hilo", "t_espera(s)", "t_navegacion(s)", "En_cola");
    fprintf(log_fp, "\n# METRICAS\n");
    fprintf(log_fp, "hilo,wait_s,browse_s,queued\n");

    double sum_wait = 0, sum_browse = 0;
    int queued_count = 0;
    for (int i = 0; i < N_users; i++) {
        printf("T%02d    %-12.3f %-14.3f %s\n",
               users[i].id, users[i].wait_time, users[i].browse_time,
               users[i].was_queued ? "SI" : "NO");
        fprintf(log_fp, "T%02d,%.3f,%.3f,%d\n",
                users[i].id, users[i].wait_time, users[i].browse_time, users[i].was_queued);
        sum_wait   += users[i].wait_time;
        sum_browse += users[i].browse_time;
        if (users[i].was_queued) queued_count++;
    }

    printf("\n--- Resumen ---\n");
    printf("Tiempo total sistema:   %.3f s\n", total_elapsed);
    printf("Usuarios en cola FIFO:  %d / %d\n", queued_count, N_users);
    printf("t_espera promedio:      %.3f s\n", sum_wait / N_users);
    printf("t_navegacion promedio:  %.3f s\n", sum_browse / N_users);
    printf("Throughput:             %.2f usuarios/s\n", N_users / total_elapsed);

    fprintf(log_fp, "\n# RESUMEN\n");
    fprintf(log_fp, "total_s,%.3f\nqueued,%d\navg_wait,%.3f\navg_browse,%.3f\nthroughput,%.2f\n",
            total_elapsed, queued_count, sum_wait/N_users, sum_browse/N_users, N_users/total_elapsed);

    sem_destroy(&slots);
    pthread_mutex_destroy(&db_mutex);
    fclose(log_fp);
    printf("\nLog guardado en: %s\n\n", LOG_FILE);
    return 0;
}
