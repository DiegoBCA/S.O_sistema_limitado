#!/bin/bash
# ============================================================
# run_all.sh - Compilar y ejecutar todos los modulos
# Universidad de las Americas Puebla (UDLAP)
# Sistemas Operativos - P26-LIS2062-1
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SRC="$PROJECT_DIR/src"
BIN="$PROJECT_DIR/bin"
RESULTS="$PROJECT_DIR/results"

mkdir -p "$BIN" "$RESULTS/logs" "$RESULTS/tables" "$RESULTS/screenshots"

echo "============================================="
echo " UDLAP - Sistemas Operativos P26-LIS2062-1"
echo " Sistema de Acceso Limitado a Pagina Web"
echo "============================================="
echo ""

# ── Dependencias ──────────────────────────────────
echo "[1/4] Verificando dependencias..."
for dep in gcc pthread; do
    if ! command -v gcc &>/dev/null; then
        echo "ERROR: gcc no encontrado. Instalar con: sudo apt install build-essential"
        exit 1
    fi
done
echo "      OK: gcc $(gcc --version | head -1)"

# ── Compilacion ───────────────────────────────────
echo ""
echo "[2/4] Compilando modulos..."

gcc -O0 -Wall -Wextra \
    -o "$BIN/modulo1_acceso" "$SRC/modulo1_acceso.c" \
    -lpthread -lrt
echo "      OK: modulo1_acceso"

gcc -O0 -Wall -Wextra \
    -o "$BIN/modulo2_race" "$SRC/modulo2_race.c" \
    -lpthread
echo "      OK: modulo2_race"

gcc -O2 -Wall -Wextra \
    -o "$BIN/modulo3_metrics" "$SRC/modulo3_metrics.c" \
    -lpthread -lm
echo "      OK: modulo3_metrics"

# ── Ejecucion ─────────────────────────────────────
cd "$RESULTS"

echo ""
echo "[3/4] Ejecutando experimentos..."
echo ""

echo "━━━━ MODULO 1: Acceso limitado con semaforo ━━━━"
echo "   Parametros: 3 slots, 8 usuarios"
"$BIN/modulo1_acceso" 3 8 42

echo ""
echo "━━━━ MODULO 2: Condicion de carrera ━━━━"
echo "   Parametros: 8 hilos, 500000 iteraciones"
"$BIN/modulo2_race" 8 500000

echo ""
echo "━━━━ MODULO 3: Metricas de CPU ━━━━"
"$BIN/modulo3_metrics"

# ── Resumen de logs ───────────────────────────────
echo ""
echo "[4/4] Generando resumen de resultados..."
echo ""
echo "Archivos generados:"
ls -lh "$RESULTS/logs/"
echo ""
echo "Para ver logs individuales:"
echo "  cat $RESULTS/logs/modulo1_access.log"
echo "  cat $RESULTS/logs/modulo2_race.log"
echo "  cat $RESULTS/logs/modulo3_metrics.log"
echo ""
echo "============================================="
echo " Ejecucion completa exitosamente"
echo "============================================="
