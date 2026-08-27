/* core/ds/lista_circular.c — lista circular com ponteiro para o último nó.
 *
 * Um ponteiro por nó, como na lista simples, e UM ponteiro só na lista — para
 * o ÚLTIMO. O último aponta para o primeiro, então guardar o fim dá o começo
 * de graça: `fim->prox` é o primeiro nó. Com isso inserir no início e inserir
 * no fim custam os dois O(1), o que a lista simples não consegue com um
 * ponteiro só.
 *
 * O preço aparece na travessia. Não existe NULL para parar: seguir prox roda
 * para sempre. Toda caminhada aqui é limitada pelo contador de elementos, e é
 * esse o erro clássico que a estrutura ensina.
 *
 * A lista não emite PTR_INICIO de propósito — ela não tem esse ponteiro. Quem
 * desenha começa a cadeia no sucessor do nó apontado por PTR_FIM, que é
 * exatamente o que o código faz para achar o primeiro.
 *
 * O algoritmo é o da matéria. As chamadas TR contam o que aconteceu para quem
 * estiver assistindo; tirá-las devolve exatamente o código de sempre. */

#define TR_SRC SRC_LISTA_CIRCULAR

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

struct ListaCircular {
    No *fim;        /* o último; fim->prox é o primeiro */
    int n;
};

ListaCircular *lista_circular_criar(void)
{
    ListaCircular *l = malloc(sizeof *l);

    if (l != NULL) {
        l->fim = NULL;
        l->n = 0;
    }
    return l;
}

/* O primeiro nó, ou NULL na lista vazia. */
static No *primeiro_no(const ListaCircular *l)
{
    return (l->fim != NULL) ? l->fim->prox : NULL;
}

/* Anda `passos` nós a partir do primeiro.
 *
 * O laço conta passos em vez de procurar NULL: aqui não existe NULL para
 * achar. Trocar esta condição por `atual != NULL` é o travamento clássico. */
static No *andar(const ListaCircular *l, int passos)
{
    No *atual = primeiro_no(l);
    int i;

    if (passos > 0) {
        TR(EV_MSG, .a = STR_ANDANDO);
    }

    for (i = 0; i < passos; i++) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        TR(EV_UNVISIT, .a = id_de(atual));
        atual = atual->prox;
        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = id_de(atual));
    }

    return atual;
}

int lista_circular_inserir(ListaCircular *l, int pos, elem_t valor)
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

    if (l->fim == NULL) {
        /* Sozinho na lista, o nó aponta para si mesmo. */
        novo->prox = novo;
        TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = id_de(novo));
        l->fim = novo;
        TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(novo));
    } else {
        /* Inserir no início e inserir no fim fazem a MESMA ligação: entre o
         * último e o primeiro. A única diferença é se o fim passa a ser o nó
         * novo — e é por isso que as duas custam o mesmo, sem andar nada. */
        anterior = (pos == 0 || pos == l->n) ? l->fim : andar(l, pos - 1);

        novo->prox = anterior->prox;
        TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = id_de(anterior->prox));
        anterior->prox = novo;
        TR(EV_EDGE_SET, .a = id_de(anterior), .b = 0, .c = id_de(novo));

        if (pos == l->n) {
            l->fim = novo;
            TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(novo));
        }
    }

    l->n++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_circular_remover(ListaCircular *l, int pos, elem_t *saida)
{
    No *morto;
    No *anterior;

    if (l->fim == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }
    if (pos < 0 || pos >= l->n) {
        TR(EV_MSG, .a = STR_POSICAO_INVALIDA);
        return ERR_ARG_INVALIDO;
    }

    /* Quem antecede o nó a remover. Para a posição 0 é o próprio fim, sem
     * andar nada: é a mesma simetria da inserção. */
    anterior = (pos == 0) ? l->fim : andar(l, pos - 1);
    morto = anterior->prox;

    TR(EV_VISIT, .a = id_de(morto));
    *saida = morto->valor;

    if (l->n == 1) {
        l->fim = NULL;
        TR(EV_PTR_SET, .a = PTR_FIM, .b = 0);
    } else {
        anterior->prox = morto->prox;
        TR(EV_EDGE_SET, .a = id_de(anterior), .b = 0, .c = id_de(morto->prox));
        if (morto == l->fim) {
            l->fim = anterior;
            TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(anterior));
        }
    }

    TR(EV_NODE_FREE, .a = id_de(morto));
    id_esquece(morto);
    free(morto);

    l->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);

    return OK;
}

int lista_circular_buscar(const ListaCircular *l, elem_t valor, int *pos)
{
    No *atual = primeiro_no(l);
    int i;

    for (i = 0; i < l->n; i++) {
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
    }

    /* Chegou aqui com atual de volta no primeiro nó: a lista inteira foi
     * vista e o próximo passo repetiria tudo. É o instante exato em que uma
     * busca escrita com `while (atual != NULL)` ficaria rodando para sempre. */
    if (l->n > 0) {
        TR(EV_MSG, .a = STR_DEU_VOLTA);
    }

    TR(EV_PTR_SET, .a = PTR_CURSOR, .b = 0);
    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

int lista_circular_primeiro(const ListaCircular *l, elem_t *saida)
{
    No *primeiro = primeiro_no(l);

    if (primeiro == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_VISIT, .a = id_de(primeiro));
    *saida = primeiro->valor;
    TR(EV_UNVISIT, .a = id_de(primeiro));

    return OK;
}

void lista_circular_limpar(ListaCircular *l)
{
    while (l->n > 0) {
        No *morto = primeiro_no(l);

        if (l->n == 1) {
            l->fim = NULL;
            TR(EV_PTR_SET, .a = PTR_FIM, .b = 0);
        } else {
            l->fim->prox = morto->prox;
            TR(EV_EDGE_SET, .a = id_de(l->fim), .b = 0, .c = id_de(morto->prox));
        }

        TR(EV_NODE_FREE, .a = id_de(morto));
        id_esquece(morto);
        free(morto);

        l->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }
}

int lista_circular_tamanho(const ListaCircular *l)
{
    return l->n;
}

void lista_circular_destruir(ListaCircular *l)
{
    if (l == NULL) {
        return;
    }
    lista_circular_limpar(l);
    free(l);
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    (void) capacidade;
    return lista_circular_criar();
}

static void vt_destruir(void *s)
{
    lista_circular_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return lista_circular_inserir(s, 0, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return lista_circular_remover(s, 0, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return lista_circular_primeiro(s, saida);
}

static void vt_limpar(void *s)
{
    lista_circular_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return lista_circular_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

static int vt_inserir_em(void *s, int pos, elem_t valor)
{
    return lista_circular_inserir(s, pos, valor);
}

static int vt_remover_em(void *s, int pos, elem_t *saida)
{
    return lista_circular_remover(s, pos, saida);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return lista_circular_buscar(s, valor, pos);
}

const TAD_Linear LISTA_CIRCULAR = {
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
