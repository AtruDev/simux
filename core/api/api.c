/* core/api/api.c — o único arquivo do core que conhece o Emscripten.
 *
 * Todo o resto compila igual nativo e para wasm, o que dá testes rápidos,
 * gdb e um binário de terminal de graça. */

#define TR_SRC SRC_API

#include "ds/api.h"

#include <stddef.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/pilha.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif

static int32_t   g_erro = OK;
static int32_t   g_tipo = TIPO_NENHUM;
static PilhaEnc *g_pilha;

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

API void ds_sessao_fim(void)
{
    if (g_pilha != NULL) {
        pilha_enc_destruir(g_pilha);
        g_pilha = NULL;
    }
    g_tipo = TIPO_NENHUM;

    /* A numeração recomeça do 1 a cada sessão: os ids são identidade visual,
     * e nada da sessão anterior continua na tela. */
    idmap_reset();
}

API int32_t ds_sessao_nova(int32_t tipo)
{
    ds_sessao_fim();
    trace_reset();
    g_erro = OK;

    switch (tipo) {
    case TIPO_PILHA_ENC:
        g_pilha = pilha_enc_criar();
        if (g_pilha == NULL) {
            return concluir(ERR_SEM_MEMORIA);
        }
        g_tipo = tipo;
        /* A tela precisa nascer com o rótulo apontando para lugar nenhum. */
        TR(EV_PTR_SET, .a = PTR_TOPO, .b = 0);
        return OK;

    default:
        return concluir(ERR_ARG_INVALIDO);
    }
}

API int32_t ds_tipo_sessao(void)
{
    return g_tipo;
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

    if (g_pilha == NULL) {
        return concluir(ERR_SEM_SESSAO);
    }

    switch (op) {
    case OP_PUSH:
        return concluir(pilha_enc_push(g_pilha, a));

    case OP_POP:
        return concluir(pilha_enc_pop(g_pilha, &descartado));

    case OP_TOPO:
        return concluir(pilha_enc_topo(g_pilha, &descartado));

    case OP_LIMPAR:
        pilha_enc_limpar(g_pilha);
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
