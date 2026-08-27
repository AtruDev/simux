/* core/ds/lista_simples.c — lista simplesmente encadeada.
 *
 * Um ponteiro por nó e um ponteiro para o início. Não existe ponteiro para o
 * fim de propósito: inserir no fim tem que andar a lista inteira, e é
 * justamente essa caminhada — visível, nó por nó — que explica por que a lista
 * dupla e a circular existem.
 *
 * O algoritmo é o da matéria. As chamadas TR contam o que aconteceu para quem
 * estiver assistindo; tirá-las devolve exatamente o código de sempre. */

#define TR_SRC SRC_LISTA_SIMPLES

#include "ds/lista.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/trace.h"

#include "linear.h"

typedef struct No {
    elem_t     valor;
    struct No *prox;
} No;

struct ListaSimples {
    No *inicio;
    int n;
};

ListaSimples *lista_simples_criar(void)
{
    ListaSimples *l = malloc(sizeof *l);

    if (l != NULL) {
        l->inicio = NULL;
        l->n = 0;
    }
    return l;
}

/* Anda até o nó da posição pedida, contando os passos.
 *
 * A contagem não é enfeite: é a medida de custo da estrutura, e é o número que
 * fica diferente do da lista dupla na mesma operação. */
static No *andar_ate(const ListaSimples *l, int pos)
{
    No *atual = l->inicio;
    int i;

    if (pos > 0) {
        TR(EV_MSG, .a = STR_ANDANDO);
    }

    for (i = 0; i < pos && atual != NULL; i++) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        TR(EV_UNVISIT, .a = id_de(atual));
        atual = atual->prox;
        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = id_de(atual));
    }

    return atual;
}

int lista_simples_inserir(ListaSimples *l, int pos, elem_t valor)
{
    No *novo;
    No *anterior;

    if (pos < 0 || pos > l->n) {
        TR(EV_MSG, .a = STR_POSICAO_INVALIDA);
        return ERR_ARG_INVALIDO;
    }

    novo = malloc(sizeof *novo);
    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }
    novo->valor = valor;
    TR(EV_NODE_NEW, .a = id_de(novo), .b = valor);

    if (pos == 0) {
        novo->prox = l->inicio;
        TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = id_de(l->inicio));
        l->inicio = novo;
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(novo));
    } else {
        /* Para inserir na posição p é preciso o nó p-1: quem vai passar a
         * apontar para o novo. É por isso que a caminhada para uma posição
         * antes do destino. */
        anterior = andar_ate(l, pos - 1);
        novo->prox = anterior->prox;
        TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = id_de(anterior->prox));
        anterior->prox = novo;
        TR(EV_EDGE_SET, .a = id_de(anterior), .b = 0, .c = id_de(novo));
    }

    l->n++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_simples_remover(ListaSimples *l, int pos, elem_t *saida)
{
    No *morto;
    No *anterior;

    if (l->inicio == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }
    if (pos < 0 || pos >= l->n) {
        TR(EV_MSG, .a = STR_POSICAO_INVALIDA);
        return ERR_ARG_INVALIDO;
    }

    if (pos == 0) {
        morto = l->inicio;
        l->inicio = morto->prox;
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(l->inicio));
    } else {
        anterior = andar_ate(l, pos - 1);
        morto = anterior->prox;
        anterior->prox = morto->prox;
        TR(EV_EDGE_SET, .a = id_de(anterior), .b = 0, .c = id_de(morto->prox));
    }

    TR(EV_VISIT, .a = id_de(morto));
    *saida = morto->valor;

    /* O evento vem antes do esquecimento: depois dele o endereço volta a ser
     * desconhecido, e id_de() devolveria um id novo para um nó que está
     * morrendo. */
    TR(EV_NODE_FREE, .a = id_de(morto));
    id_esquece(morto);
    free(morto);

    l->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_simples_buscar(const ListaSimples *l, elem_t valor, int *pos)
{
    No *atual = l->inicio;
    int i = 0;

    while (atual != NULL) {
        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = id_de(atual));
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (atual->valor == valor) {
            TR(EV_MSG, .a = STR_ACHOU);
            *pos = i;
            return OK;
        }

        TR(EV_UNVISIT, .a = id_de(atual));
        atual = atual->prox;
        i++;
    }

    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);
    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

int lista_simples_primeiro(const ListaSimples *l, elem_t *saida)
{
    if (l->inicio == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_VISIT, .a = id_de(l->inicio));
    *saida = l->inicio->valor;
    TR(EV_UNVISIT, .a = id_de(l->inicio));

    return OK;
}

void lista_simples_limpar(ListaSimples *l)
{
    while (l->inicio != NULL) {
        No *morto = l->inicio;

        l->inicio = morto->prox;
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(l->inicio));
        TR(EV_NODE_FREE, .a = id_de(morto));
        id_esquece(morto);
        free(morto);

        l->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }
}

int lista_simples_tamanho(const ListaSimples *l)
{
    return l->n;
}

void lista_simples_destruir(ListaSimples *l)
{
    if (l == NULL) {
        return;
    }
    lista_simples_limpar(l);
    free(l);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * As assinaturas do TAD_Linear são sobre void*, então cada implementação
 * expõe uma casca fina. Fica no fim do arquivo para não atrapalhar a leitura
 * do algoritmo, que é o que aparece no painel de código.                    */

static void *vt_criar(int capacidade)
{
    (void) capacidade;      /* a encadeada não tem limite */
    return lista_simples_criar();
}

static void vt_destruir(void *s)
{
    lista_simples_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return lista_simples_inserir(s, 0, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return lista_simples_remover(s, 0, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return lista_simples_primeiro(s, saida);
}

static void vt_limpar(void *s)
{
    lista_simples_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return lista_simples_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

static int vt_inserir_em(void *s, int pos, elem_t valor)
{
    return lista_simples_inserir(s, pos, valor);
}

static int vt_remover_em(void *s, int pos, elem_t *saida)
{
    return lista_simples_remover(s, pos, saida);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return lista_simples_buscar(s, valor, pos);
}

const TAD_Linear LISTA_SIMPLES = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .remover = vt_remover,
    .consultar = vt_consultar,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .inserir_em = vt_inserir_em,
    .remover_em = vt_remover_em,
    .buscar = vt_buscar,
};
