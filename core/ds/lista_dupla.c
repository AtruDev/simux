/* core/ds/lista_dupla.c — lista duplamente encadeada.
 *
 * Dois ponteiros por nó (prox e ant) e dois ponteiros na lista (início e fim).
 * O preço é memória: cada nó ocupa um ponteiro a mais que o da lista simples.
 * O que se compra com ele são duas coisas concretas, e as duas aparecem na
 * animação:
 *
 *   - inserir e remover no fim são O(1), sem andar a lista;
 *   - a travessia começa pela ponta mais PERTO da posição pedida, então o pior
 *     caso é n/2 passos em vez de n.
 *
 * Nos eventos, o slot 0 é prox e o slot 1 é ant. É o mesmo EV_EDGE_SET da
 * lista simples, com um slot a mais — nenhum evento novo foi preciso.
 *
 * O algoritmo é o da matéria. As chamadas TR contam o que aconteceu para quem
 * estiver assistindo; tirá-las devolve exatamente o código de sempre. */

#define TR_SRC SRC_LISTA_DUPLA

#include "ds/lista.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/trace.h"

#include "linear.h"

enum { SLOT_PROX = 0, SLOT_ANT = 1 };

typedef struct No {
    elem_t     valor;
    struct No *prox;
    struct No *ant;
} No;

struct ListaDupla {
    No *inicio;
    No *fim;
    int n;
};

ListaDupla *lista_dupla_criar(void)
{
    ListaDupla *l = malloc(sizeof *l);

    if (l != NULL) {
        l->inicio = NULL;
        l->fim = NULL;
        l->n = 0;
    }
    return l;
}

/* Anda até o nó da posição pedida, pela ponta mais perto.
 *
 * É a vantagem que justifica o ponteiro a mais por nó: buscar a posição n-1
 * custa um passo, não n. O contador de comparações mostra a diferença contra
 * a lista simples na mesma operação. */
static No *andar_ate(const ListaDupla *l, int pos)
{
    No *atual;
    int i;

    if (pos > 0 && pos < l->n - 1) {
        TR(EV_MSG, .a = STR_ANDANDO);
    }

    if (pos <= l->n / 2) {
        atual = l->inicio;
        for (i = 0; i < pos && atual != NULL; i++) {
            TR(EV_VISIT, .a = id_de(atual));
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            TR(EV_UNVISIT, .a = id_de(atual));
            atual = atual->prox;
            TR(EV_PTR_SET, .a = PTR_CURSOR, .b = id_de(atual));
        }
    } else {
        atual = l->fim;
        for (i = l->n - 1; i > pos && atual != NULL; i--) {
            TR(EV_VISIT, .a = id_de(atual));
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            TR(EV_UNVISIT, .a = id_de(atual));
            atual = atual->ant;
            TR(EV_PTR_SET, .a = PTR_CURSOR, .b = id_de(atual));
        }
    }

    return atual;
}

/* Liga a e b nos dois sentidos, aceitando NULL de qualquer lado. */
static void ligar(No *a, No *b)
{
    if (a != NULL) {
        a->prox = b;
        TR(EV_EDGE_SET, .a = id_de(a), .b = SLOT_PROX, .c = id_de(b));
    }
    if (b != NULL) {
        b->ant = a;
        TR(EV_EDGE_SET, .a = id_de(b), .b = SLOT_ANT, .c = id_de(a));
    }
}

int lista_dupla_inserir(ListaDupla *l, int pos, elem_t valor)
{
    No *novo;
    No *depois;

    if (pos < 0 || pos > l->n) {
        TR(EV_MSG, .a = STR_POSICAO_INVALIDA);
        return ERR_ARG_INVALIDO;
    }

    novo = malloc(sizeof *novo);
    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }
    novo->valor = valor;
    novo->prox = NULL;
    novo->ant = NULL;
    TR(EV_NODE_NEW, .a = id_de(novo), .b = valor);

    /* Quem vai ficar DEPOIS do novo. Nulo quer dizer "no fim", e é o caso que
     * não anda a lista: o ponteiro de fim já sabe onde é. */
    depois = (pos == l->n) ? NULL : andar_ate(l, pos);

    if (depois == NULL) {
        ligar(l->fim, novo);
        l->fim = novo;
        TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(novo));
    } else {
        ligar(depois->ant, novo);
        ligar(novo, depois);
    }

    if (pos == 0) {
        l->inicio = novo;
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(novo));
    }

    l->n++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_dupla_remover(ListaDupla *l, int pos, elem_t *saida)
{
    No *morto;

    if (l->inicio == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }
    if (pos < 0 || pos >= l->n) {
        TR(EV_MSG, .a = STR_POSICAO_INVALIDA);
        return ERR_ARG_INVALIDO;
    }

    morto = andar_ate(l, pos);
    TR(EV_VISIT, .a = id_de(morto));
    *saida = morto->valor;

    /* Os dois vizinhos passam a se ver direto. Com um deles nulo, ligar() não
     * faz nada daquele lado — e é aí que o ponteiro da lista precisa mudar. */
    ligar(morto->ant, morto->prox);

    if (morto == l->inicio) {
        l->inicio = morto->prox;
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(l->inicio));
    }
    if (morto == l->fim) {
        l->fim = morto->ant;
        TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(l->fim));
    }

    TR(EV_NODE_FREE, .a = id_de(morto));
    id_esquece(morto);
    free(morto);

    l->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_dupla_buscar(const ListaDupla *l, elem_t valor, int *pos)
{
    No *atual = l->inicio;
    int i = 0;

    /* A busca por valor anda do começo mesmo: sem saber onde o valor está, a
     * segunda ponta não ajuda. */
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

int lista_dupla_primeiro(const ListaDupla *l, elem_t *saida)
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

void lista_dupla_limpar(ListaDupla *l)
{
    while (l->inicio != NULL) {
        No *morto = l->inicio;

        l->inicio = morto->prox;
        if (l->inicio != NULL) {
            l->inicio->ant = NULL;
            TR(EV_EDGE_SET, .a = id_de(l->inicio), .b = SLOT_ANT, .c = 0);
        }
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(l->inicio));
        TR(EV_NODE_FREE, .a = id_de(morto));
        id_esquece(morto);
        free(morto);

        l->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }

    l->fim = NULL;
    TR(EV_PTR_SET, .a = PTR_FIM, .b = 0);
}

int lista_dupla_tamanho(const ListaDupla *l)
{
    return l->n;
}

void lista_dupla_destruir(ListaDupla *l)
{
    if (l == NULL) {
        return;
    }
    lista_dupla_limpar(l);
    free(l);
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    (void) capacidade;
    return lista_dupla_criar();
}

static void vt_destruir(void *s)
{
    lista_dupla_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return lista_dupla_inserir(s, 0, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return lista_dupla_remover(s, 0, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return lista_dupla_primeiro(s, saida);
}

static void vt_limpar(void *s)
{
    lista_dupla_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return lista_dupla_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

static int vt_inserir_em(void *s, int pos, elem_t valor)
{
    return lista_dupla_inserir(s, pos, valor);
}

static int vt_remover_em(void *s, int pos, elem_t *saida)
{
    return lista_dupla_remover(s, pos, saida);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return lista_dupla_buscar(s, valor, pos);
}

const TAD_Linear LISTA_DUPLA = {
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
