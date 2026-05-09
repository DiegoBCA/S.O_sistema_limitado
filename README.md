# S.O_sistema_limitado
# Sistema de Acceso Limitado a Página Web
## Universidad de las Américas Puebla (UDLAP)
### Sistemas Operativos · P26-LIS2062-1

**Integrantes:**
- Daniel de Jesús Martínez Gallegos
- Diego Bedolla Carrillo

---

## Descripción

Simulación de un sistema concurrente que controla el acceso a una página web con capacidad limitada de usuarios simultáneos. Implementa semáforo contador + mutex para garantizar exclusión mutua, orden FIFO y ausencia de condiciones de carrera.

---

## Dependencias

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install build-essential gcc make

# Verificar
gcc --version    # >= 9.0
```

**Librerías:** `pthread` (POSIX threads), `rt` (POSIX real-time), `m` (math) — incluidas en glibc estándar de Linux.

---

## Estructura del Repositorio

```
project/
├── src/
│   ├── modulo1_acceso.c     # Semáforo + mutex: acceso limitado
│   ├── modulo2_race.c       # Condición de carrera antes/después de mutex
│   └── modulo3_metrics.c    # Métricas de CPU: turnaround, throughput
├── scripts/
│   ├── run_all.sh           # Compila y ejecuta todo
│   └── experiment_race.sh   # Experimento comparativo de race condition
├── results/
│   ├── logs/                # Logs generados en ejecución
│   ├── tables/              # Tablas CSV de métricas
│   └── screenshots/         # Evidencia visual
└── README.md
```

---

## Compilación

```bash
# Desde la raíz del proyecto
mkdir -p bin results/logs results/tables

# Módulo 1: acceso limitado
gcc -O0 -Wall -o bin/modulo1_acceso src/modulo1_acceso.c -lpthread -lrt

# Módulo 2: condición de carrera
gcc -O0 -Wall -o bin/modulo2_race src/modulo2_race.c -lpthread

# Módulo 3: métricas de CPU
gcc -O2 -Wall -o bin/modulo3_metrics src/modulo3_metrics.c -lpthread -lm
```

> **Nota:** `-O0` en módulo 2 es intencional: desactiva optimizaciones para que la condición de carrera sea observable.

---

## Ejecución

### Opción A — Script automático (recomendado)

```bash
chmod +x scripts/run_all.sh
bash scripts/run_all.sh
```

### Opción B — Módulos individuales

```bash
# Módulo 1: [N_slots] [N_usuarios] [seed]
./bin/modulo1_acceso 3 8 42

# Módulo 2: [N_hilos] [N_iteraciones]
./bin/modulo2_race 8 500000

# Módulo 3: sin parámetros
./bin/modulo3_metrics
```

### Opción C — Experimento de race condition con distintos hilos

```bash
bash scripts/experiment_race.sh
```

---

## Resultados Esperados

### Módulo 1 — Acceso limitado (N=3, usuarios=8)

```
ACCESO_CONCEDIDO | T01 | activos=1 | slots_libres=2
ACCESO_CONCEDIDO | T02 | activos=2 | slots_libres=1
ACCESO_CONCEDIDO | T03 | activos=3 | slots_libres=0
COLA_FIFO_ESPERA | T04 | activos=3 | slots_libres=0   ← bloqueado por semáforo
PAGINA_LIBERADA  | T01 | activos=2 | slots_libres=1
ACCESO_CONCEDIDO | T04 | activos=3 | slots_libres=0   ← sem_post desbloquea
...
Usuarios en cola FIFO: 5 / 8
t_espera promedio:     0.662 s
Throughput:            1.52 usuarios/s
```

### Módulo 2 — Race condition

```
SIN mutex: resultado = 2,725,548  (esperado: 4,000,000) → 31.86% escrituras corruptas
CON mutex: resultado = 4,000,000  (esperado: 4,000,000) → 0.00%  escrituras corruptas
5 corridas sin mutex: valores DIFERENTES cada vez (no-determinismo)
```

### Módulo 3 — Métricas de CPU

```
Carga homogénea  (8 hilos, burst=10ms): throughput ~593 tareas/s
Carga heterogénea (8 hilos, bursts 1-30ms): throughput ~364 tareas/s
```

---

## Conceptos de SO Demostrados

| Concepto | Módulo | Mecanismo |
|---|---|---|
| Semáforo contador | 1 | `sem_t` — bloquea cuando slots = 0 |
| Mutex / exclusión mutua | 1, 2 | `pthread_mutex_t` — sección crítica atómica |
| Condición de carrera | 2 | read-modify-write sin protección |
| FIFO / inanición | 1 | Política NPTL garantiza orden |
| Deadlock prevention | 1 | Jerarquía mutex < sem_post |
| Turnaround / throughput | 3 | Métricas de planificación |

---

## Logs Generados

```
results/logs/modulo1_access.log   — eventos de acceso con timestamps
results/logs/modulo2_race.log     — métricas de condición de carrera
results/logs/modulo3_metrics.log  — métricas de CPU por escenario
```
