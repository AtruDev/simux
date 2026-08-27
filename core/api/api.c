/* core/api/api.c — o único arquivo do core que conhece o Emscripten.
 *
 * Todo o resto compila igual nativo e para wasm, o que dá testes rápidos,
 * gdb e um binário de terminal de graça.
 *
 * Desde que existe o vtable TAD_Linear, esta camada não sabe qual estrutura
 * está do outro lado: ela repassa inserir, remover e consultar. Acrescentar a
 * lista dupla não vai mudar uma linha daqui.
 *
 * As sessões são um vetor de slots, e não uma só, porque o modo comparar roda
 * a mesma sequência nas duas implementações do mesmo TAD. ds_call continua
 * sendo (op, a, b, c): o slot não entra na chamada, escolhe-se antes com
 * ds_sessao_slot. Pôr o slot no ds_call custaria o quinto inteiro da fronteira
 * para uma informação que muda uma vez por operação, não por chamada. */

#define TR_SRC SRC_API

#include "ds/api.h"

#include <stddef.h>

#include "ds/erros.h"
#include "ds/fila.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/lista.h"
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

enum { SESSOES = 2 };

typedef struct {
    const TAD_Linear *tad;
    void             *estrutura;
    int32_t           tipo;
} Sessao;

static Sessao  g_sessao[SESSOES];
static int32_t g_ativa = 0;
static int32_t g_erro  = OK;

#define ATIVA (g_sessao[g_ativa])

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
    case TIPO_LISTA_SIMPLES:  return &LISTA_SIMPLES;
    case TIPO_LISTA_DUPLA:    return &LISTA_DUPLA;
    case TIPO_LISTA_CIRCULAR: return &LISTA_CIRCULAR;
    default:             return NULL;
    }
}

/* Verdadeiro enquanto qualquer slot ainda tiver estrutura viva. */
static int alguma_aberta(void)
{
    int i;

    for (i = 0; i < SESSOES; i++) {
        if (g_sessao[i].estrutura != NULL) {
            return 1;
        }
    }
    return 0;
}

/* Escolhe o slot sobre o qual as próximas chamadas operam. */
API int32_t ds_sessao_slot(int32_t slot)
{
    if (slot < 0 || slot >= SESSOES) {
        g_erro = ERR_ARG_INVALIDO;
        return -1;
    }
    g_ativa = slot;
    return OK;
}

API int32_t ds_sessao_slots(void)
{
    return SESSOES;
}

API void ds_sessao_fim(void)
{
    if (ATIVA.estrutura != NULL && ATIVA.tad != NULL) {
        ATIVA.tad->destruir(ATIVA.estrutura);
    }
    ATIVA.estrutura = NULL;
    ATIVA.tad = NULL;
    ATIVA.tipo = TIPO_NENHUM;

    /* A numeração recomeça do 1 a cada sessão: os ids são identidade visual, e
     * nada da sessão anterior continua na tela.
     *
     * Mas só quando a última fecha. Zerar com o outro slot aberto renumeraria
     * nós que continuam na tela — os ids que o frontend já recebeu deixariam
     * de valer, e a comparação passaria a desenhar sobre o nó errado. */
    if (!alguma_aberta()) {
        idmap_reset();
    }
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
    ATIVA.estrutura = tad->criar(capacidade);
    if (ATIVA.estrutura == NULL) {
        return concluir(ERR_SEM_MEMORIA);
    }

    ATIVA.tad = tad;
    ATIVA.tipo = tipo;
    return OK;
}

API int32_t ds_tipo_sessao(void)
{
    return ATIVA.tipo;
}

/* Tamanho de verdade da estrutura, que não é o do modelo do frontend: aquele
 * reflete onde a animação está, e este reflete tudo o que já foi executado.
 * Quem precisa inserir "no fim" precisa deste. */
API int32_t ds_tamanho(void)
{
    return (ATIVA.tad != NULL) ? ATIVA.tad->tamanho(ATIVA.estrutura) : 0;
}

API int32_t ds_capacidade(void)
{
    return (ATIVA.tad != NULL) ? ATIVA.tad->capacidade(ATIVA.estrutura) : -1;
}

API int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c)
{
    elem_t descartado;
    int    posicao = 0;

    (void) c;

    trace_reset();
    g_erro = OK;

    if (op == OP_PING) {
        TR(EV_MSG, .a = STR_PING);
        return OK;
    }

    if (ATIVA.tad == NULL) {
        return concluir(ERR_SEM_SESSAO);
    }

    switch (op) {
    case OP_PUSH:
        return concluir(ATIVA.tad->inserir(ATIVA.estrutura, a));

    case OP_POP:
        return concluir(ATIVA.tad->remover(ATIVA.estrutura, &descartado));

    case OP_TOPO:
        return concluir(ATIVA.tad->consultar(ATIVA.estrutura, &descartado));

    case OP_LIMPAR:
        ATIVA.tad->limpar(ATIVA.estrutura);
        return OK;

    /* As três com posição existem só em quem tem posição. O ponteiro nulo no
     * vtable é a resposta: pedir isso a uma pilha é operação desconhecida, não
     * argumento inválido. */
    case OP_INSERIR_EM:
        if (ATIVA.tad->inserir_em == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->inserir_em(ATIVA.estrutura, b, a));

    case OP_REMOVER_EM:
        if (ATIVA.tad->remover_em == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->remover_em(ATIVA.estrutura, b, &descartado));

    case OP_BUSCAR:
        if (ATIVA.tad->buscar == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->buscar(ATIVA.estrutura, a, &posicao));

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
