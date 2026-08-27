/* core/ds/pilha_vet.c — pilha sobre vetor, de capacidade fixa.
 *
 * É a mesma pilha da outra implementação, e mostra coisas que a encadeada não
 * mostra: a capacidade acabando, o overflow, e as posições que continuam
 * reservadas depois de um pop. Em troca, some o malloc por elemento.
 *
 * Nenhum evento novo foi preciso. Onde a encadeada emite EV_NODE_NEW, esta
 * emite EV_ARR_WRITE; onde aquela aponta EV_PTR_SET para um id de nó, esta
 * aponta para um índice. */

#define TR_SRC SRC_PILHA_VET

#include "ds/pilha.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "linear.h"

struct PilhaVet {
    elem_t *dados;
    int     cap;
    int     n;      /* também é o índice da próxima posição livre */
};

PilhaVet *pilha_vet_criar(int capacidade)
{
    PilhaVet *p;

    if (capacidade <= 0) {
        return NULL;
    }

    p = malloc(sizeof *p);
    if (p == NULL) {
        return NULL;
    }

    p->dados = malloc((size_t) capacidade * sizeof *p->dados);
    if (p->dados == NULL) {
        free(p);
        return NULL;
    }

    p->cap = capacidade;
    p->n = 0;

    /* A tela precisa saber quantas células desenhar antes de haver dados. */
    TR(EV_ARR_INIT, .a = capacidade);
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = -1);

    return p;
}

int pilha_vet_push(PilhaVet *p, elem_t valor)
{
    if (p->n == p->cap) {
        /* O erro que esta implementação ensina, e que a encadeada não tem. */
        TR(EV_MSG, .a = STR_PILHA_CHEIA);
        return ERR_CHEIA;
    }

    p->dados[p->n] = valor;
    TR(EV_ARR_WRITE, .a = p->n, .b = valor);

    p->n++;
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = p->n - 1);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);

    return OK;
}

int pilha_vet_pop(PilhaVet *p, elem_t *saida)
{
    if (p->n == 0) {
        TR(EV_MSG, .a = STR_PILHA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = p->n - 1);
    *saida = p->dados[p->n - 1];

    p->n--;
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = p->n - 1);

    /* O valor continua na memória; o que mudou foi só o topo. Marcar a célula
     * como livre é justamente o que torna isso visível — e é a diferença
     * concreta para o free da versão encadeada. */
    TR(EV_ARR_MARK, .a = p->n, .b = TAG_LIVRE);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);

    return OK;
}

int pilha_vet_topo(const PilhaVet *p, elem_t *saida)
{
    if (p->n == 0) {
        TR(EV_MSG, .a = STR_PILHA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = p->n - 1);
    *saida = p->dados[p->n - 1];

    return OK;
}

void pilha_vet_limpar(PilhaVet *p)
{
    while (p->n > 0) {
        p->n--;
        TR(EV_ARR_MARK, .a = p->n, .b = TAG_LIVRE);
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }
    TR(EV_PTR_SET, .a = PTR_TOPO, .b = -1);
}

int pilha_vet_tamanho(const PilhaVet *p)
{
    return p->n;
}

int pilha_vet_capacidade(const PilhaVet *p)
{
    return p->cap;
}

void pilha_vet_destruir(PilhaVet *p)
{
    if (p == NULL) {
        return;
    }
    free(p->dados);
    free(p);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * As assinaturas do TAD_Linear são sobre void*, então cada implementação
 * expõe uma casca fina. Fica tudo no fim do arquivo para não atrapalhar a
 * leitura do algoritmo, que é o que aparece no painel de código.           */

static void *vt_criar(int capacidade)
{
    return pilha_vet_criar(capacidade);
}

static void vt_destruir(void *s)
{
    pilha_vet_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return pilha_vet_push(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return pilha_vet_pop(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return pilha_vet_topo(s, saida);
}

static void vt_limpar(void *s)
{
    pilha_vet_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return pilha_vet_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return pilha_vet_capacidade(s);
}

const TAD_Linear PILHA_VET = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .remover = vt_remover,
    .consultar = vt_consultar,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    /* sem posição: os três de baixo ficam nulos, e api.c devolve
       ERR_OP_DESCONHECIDA para quem pedir */
};
