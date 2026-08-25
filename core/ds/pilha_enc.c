/* core/ds/pilha_enc.c — pilha com alocação dinâmica.
 *
 * O algoritmo é o da matéria. As chamadas TR contam o que aconteceu para quem
 * estiver assistindo; tirá-las devolve exatamente o código de sempre. */

#define TR_SRC SRC_PILHA_ENC

#include "ds/pilha.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/trace.h"

#include "linear.h"

typedef struct No {
    elem_t      valor;
    struct No  *prox;
} No;

struct PilhaEnc {
    No *topo;
    int n;
};

PilhaEnc *pilha_enc_criar(void)
{
    PilhaEnc *p = malloc(sizeof *p);

    if (p != NULL) {
        p->topo = NULL;
        p->n = 0;
    }
    return p;
}

int pilha_enc_push(PilhaEnc *p, elem_t valor)
{
    No *novo = malloc(sizeof *novo);

    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }

    novo->valor = valor;
    novo->prox = p->topo;
    TR(EV_NODE_NEW, .a = id_de(novo), .b = valor);
    TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = id_de(p->topo));

    p->topo = novo;
    p->n++;
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = id_de(novo));
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    return OK;
}

int pilha_enc_pop(PilhaEnc *p, elem_t *saida)
{
    No *morto;

    if (p->topo == NULL) {
        TR(EV_MSG, .a = STR_PILHA_VAZIA);
        return ERR_VAZIA;
    }

    morto = p->topo;
    TR(EV_VISIT, .a = id_de(morto));
    *saida = morto->valor;

    p->topo = morto->prox;
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = id_de(p->topo));

    /* O evento vem antes do esquecimento: depois dele o endereço volta a ser
     * desconhecido, e id_de() devolveria um id novo para um nó que está
     * morrendo. */
    TR(EV_NODE_FREE, .a = id_de(morto));
    id_esquece(morto);
    free(morto);

    p->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);

    return OK;
}

int pilha_enc_topo(const PilhaEnc *p, elem_t *saida)
{
    if (p->topo == NULL) {
        TR(EV_MSG, .a = STR_PILHA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_VISIT, .a = id_de(p->topo));
    *saida = p->topo->valor;
    TR(EV_UNVISIT, .a = id_de(p->topo));

    return OK;
}

void pilha_enc_limpar(PilhaEnc *p)
{
    while (p->topo != NULL) {
        No *morto = p->topo;

        p->topo = morto->prox;
        TR(EV_PTR_SET, .a = PTR_TOPO, .b = id_de(p->topo));
        TR(EV_NODE_FREE, .a = id_de(morto));
        id_esquece(morto);
        free(morto);

        p->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }
}

int pilha_enc_tamanho(const PilhaEnc *p)
{
    return p->n;
}

void pilha_enc_destruir(PilhaEnc *p)
{
    if (p == NULL) {
        return;
    }
    pilha_enc_limpar(p);
    free(p);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * As assinaturas do TAD_Linear são sobre void*, então cada implementação
 * expõe uma casca fina. Fica no fim do arquivo para não atrapalhar a leitura
 * do algoritmo, que é o que aparece no painel de código.                    */

static void *vt_criar(int capacidade)
{
    (void) capacidade;      /* a encadeada não tem limite */
    return pilha_enc_criar();
}

static void vt_destruir(void *s)
{
    pilha_enc_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return pilha_enc_push(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return pilha_enc_pop(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return pilha_enc_topo(s, saida);
}

static void vt_limpar(void *s)
{
    pilha_enc_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return pilha_enc_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

const TAD_Linear PILHA_ENC = {
    vt_criar, vt_destruir,
    vt_inserir, vt_remover, vt_consultar, vt_limpar,
    vt_tamanho, vt_capacidade,
};
