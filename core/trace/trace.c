/* core/trace/trace.c — o buffer de eventos.
 *
 * Um vetor estático e um índice. Não há alocação aqui de propósito: o buffer
 * precisa sobreviver a qualquer falta de memória durante a operação, e o
 * ponteiro devolvido por trace_ptr() tem que continuar válido enquanto o JS
 * lê a heap do wasm. */

#include "ds/trace.h"

static ev_t    g_ev[TRACE_CAP];
static int32_t g_n;
static int32_t g_truncado;
static int     g_ligado = 1;

void trace_reset(void)
{
    g_n = 0;
    g_truncado = 0;
}

void trace_push(ev_t e)
{
    if (!g_ligado) {
        return;
    }
    if (g_n >= (int32_t) TRACE_CAP) {
        /* Perder eventos em silêncio seria pior que truncar: a animação
         * ficaria errada sem ninguém saber. A interface avisa. */
        g_truncado = 1;
        return;
    }
    g_ev[g_n++] = e;
}

int32_t trace_len(void)
{
    return g_n;
}

const ev_t *trace_ptr(void)
{
    return g_ev;
}

int32_t trace_truncado(void)
{
    return g_truncado;
}

void trace_set_enabled(int ligado)
{
    g_ligado = ligado ? 1 : 0;
}

int trace_enabled(void)
{
    return g_ligado;
}
