/* core/api/api.c — o único arquivo do core que conhece o Emscripten.
 *
 * Todo o resto compila igual nativo e para wasm, o que dá testes rápidos,
 * gdb e um binário de terminal de graça.
 *
 * Desde que existe o vtable TAD_Linear, esta camada não sabe qual estrutura
 * está do outro lado: ela repassa inserir, remover e consultar. Acrescentar a
 * lista dupla não vai mudar uma linha daqui. */

#define TR_SRC SRC_API

#include "ds/api.h"

#include <stddef.h>

#include "ds/erros.h"
#include "ds/fila.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/pilha.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "../ds/linear.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif

static int32_t           g_erro = OK;
static int32_t           g_tipo = TIPO_NENHUM;
static const TAD_Linear *g_tad;
static void             *g_estrutura;

/* As funções das estruturas devolvem OK ou um ERR_*; a fronteira devolve -1 e
 * guarda o motivo, porque é isso que o JS sabe testar. */
static int32_t concluir(int rc)
{
    if (rc != OK) {
        g_erro = rc;
        return -1;
    }
    return OK;
}

static const TAD_Linear *tad_de(int32_t tipo)
{
    switch (tipo) {
    case TIPO_PILHA_ENC: return &PILHA_ENC;
    case TIPO_PILHA_VET: return &PILHA_VET;
    case TIPO_FILA_ENC:  return &FILA_ENC;
    case TIPO_FILA_VET:  return &FILA_VET;
    default:             return NULL;
    }
}

API void ds_sessao_fim(void)
{
    if (g_estrutura != NULL && g_tad != NULL) {
        g_tad->destruir(g_estrutura);
    }
    g_estrutura = NULL;
    g_tad = NULL;
    g_tipo = TIPO_NENHUM;

    /* A numeração recomeça do 1 a cada sessão: os ids são identidade visual,
     * e nada da sessão anterior continua na tela. */
    idmap_reset();
}

API int32_t ds_sessao_nova(int32_t tipo, int32_t capacidade)
{
    const TAD_Linear *tad = tad_de(tipo);

    ds_sessao_fim();
    trace_reset();
    g_erro = OK;

    if (tad == NULL) {
        return concluir(ERR_ARG_INVALIDO);
    }

    /* A criação já emite eventos — EV_ARR_INIT e os ponteiros iniciais —,
     * então o trace precisa estar zerado antes dela, e não depois. */
    g_estrutura = tad->criar(capacidade);
    if (g_estrutura == NULL) {
        return concluir(ERR_SEM_MEMORIA);
    }

    g_tad = tad;
    g_tipo = tipo;
    return OK;
}

API int32_t ds_tipo_sessao(void)
{
    return g_tipo;
}

API int32_t ds_capacidade(void)
{
    return (g_tad != NULL) ? g_tad->capacidade(g_estrutura) : -1;
}

API int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c)
{
    elem_t descartado;

    (void) b;
    (void) c;

    trace_reset();
    g_erro = OK;

    if (op == OP_PING) {
        TR(EV_MSG, .a = STR_PING);
        return OK;
    }

    if (g_tad == NULL) {
        return concluir(ERR_SEM_SESSAO);
    }

    switch (op) {
    case OP_PUSH:
        return concluir(g_tad->inserir(g_estrutura, a));

    case OP_POP:
        return concluir(g_tad->remover(g_estrutura, &descartado));

    case OP_TOPO:
        return concluir(g_tad->consultar(g_estrutura, &descartado));

    case OP_LIMPAR:
        g_tad->limpar(g_estrutura);
        return OK;

    default:
        return concluir(ERR_OP_DESCONHECIDA);
    }
}

API int32_t ds_erro(void)
{
    return g_erro;
}

API const ev_t *ds_trace_ptr(void)
{
    return trace_ptr();
}

API int32_t ds_trace_len(void)
{
    return trace_len();
}

API int32_t ds_trace_truncado(void)
{
    return trace_truncado();
}
