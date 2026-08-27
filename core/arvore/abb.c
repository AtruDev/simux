/* core/arvore/abb.c — árvore binária de busca.
 *
 * O algoritmo é o da matéria, iterativo onde a aula é iterativa e recursivo
 * onde ela é recursiva. As chamadas TR contam o que aconteceu para quem estiver
 * assistindo; tirá-las devolve exatamente o código de sempre.
 *
 * Nos eventos, o slot 0 é o filho ESQUERDO e o slot 1 é o DIREITO. É o mesmo
 * EV_EDGE_SET da lista dupla, com os slots querendo dizer outra coisa — e é
 * por isso que o vocabulário de eventos não cresceu para a árvore chegar. O
 * frontend sabe ler os dois porque sabe o tipo da sessão.
 *
 * A remoção é a razão de esta estrutura estar aqui. Os dois primeiros casos
 * são triviais; o terceiro é onde todo mundo trava, e é o único lugar do
 * arquivo com uma busca dentro de outra: achar o nó, e depois achar o sucessor
 * dele. Cada passo dessa segunda descida emite EV_VISIT, porque ver a descida
 * é a diferença entre a regra decorada e a regra entendida. */

#define TR_SRC SRC_ABB

#include "ds/arvore.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/trace.h"

#include "linear.h"

enum { SLOT_ESQ = 0, SLOT_DIR = 1 };

typedef struct No {
    elem_t     valor;
    struct No *esq;
    struct No *dir;
} No;

struct Abb {
    No *raiz;
    int n;
};

Abb *abb_criar(void)
{
    Abb *a = malloc(sizeof *a);

    if (a != NULL) {
        a->raiz = NULL;
        a->n = 0;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    }
    return a;
}

/* Cria o nó já ligado a ninguém, e anuncia os dois filhos nulos.
 *
 * Anunciar os nulos não é redundância: o desenho precisa saber que o nó tem
 * dois lugares vazios, e não que ele não tem lugar nenhum. */
static No *no_novo(elem_t valor)
{
    No *n = malloc(sizeof *n);

    if (n == NULL) {
        return NULL;
    }
    n->valor = valor;
    n->esq = NULL;
    n->dir = NULL;

    TR(EV_NODE_NEW, .a = id_de(n), .b = valor);
    TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_ESQ, .c = 0);
    TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = 0);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    return n;
}

/* A comparação é a unidade de custo da árvore, e é ela que o painel conta.
 *
 * Com a árvore equilibrada são log n; com a árvore degenerada em lista, são n.
 * É a mesma sequência crescente que a AVL vai receber depois, e comparar os
 * dois números é o argumento inteiro de a AVL existir. */

int abb_inserir(Abb *a, elem_t valor)
{
    No  *atual = a->raiz;
    No  *pai = NULL;
    int  lado = SLOT_ESQ;
    No  *novo;

    while (atual != NULL) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (valor == atual->valor) {
            /* Repetido não entra: sem critério de desempate, ele tornaria a
             * remoção ambígua. Dizer isso é melhor que recusar em silêncio. */
            TR(EV_MSG, .a = STR_JA_EXISTE);
            TR(EV_UNVISIT, .a = id_de(atual));
            return OK;
        }

        pai = atual;
        if (valor < atual->valor) {
            TR(EV_MSG, .a = STR_VAI_ESQ);
            lado = SLOT_ESQ;
            atual = atual->esq;
        } else {
            TR(EV_MSG, .a = STR_VAI_DIR);
            lado = SLOT_DIR;
            atual = atual->dir;
        }
        TR(EV_UNVISIT, .a = id_de(pai));
    }

    novo = no_novo(valor);
    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }

    if (pai == NULL) {
        a->raiz = novo;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(novo));
    } else if (lado == SLOT_ESQ) {
        pai->esq = novo;
        TR(EV_EDGE_SET, .a = id_de(pai), .b = SLOT_ESQ, .c = id_de(novo));
    } else {
        pai->dir = novo;
        TR(EV_EDGE_SET, .a = id_de(pai), .b = SLOT_DIR, .c = id_de(novo));
    }

    a->n++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    return OK;
}

int abb_buscar(const Abb *a, elem_t valor, int *profundidade)
{
    const No *atual = a->raiz;
    int       nivel = 0;

    while (atual != NULL) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (valor == atual->valor) {
            TR(EV_MSG, .a = STR_ACHOU);
            *profundidade = nivel;
            return OK;
        }

        TR(EV_MSG, .a = (valor < atual->valor) ? STR_VAI_ESQ : STR_VAI_DIR);
        TR(EV_UNVISIT, .a = id_de(atual));
        atual = (valor < atual->valor) ? atual->esq : atual->dir;
        nivel++;
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

/* ---- remoção: os três casos --------------------------------------------
 *
 * O nó a remover pode ter zero, um ou dois filhos, e cada um pede uma coisa:
 *
 *   zero    o pai passa a apontar para NULL, e o nó é liberado
 *   um      o filho único sobe para o lugar dele
 *   dois    o SUCESSOR EM ORDEM — o menor da subárvore direita — toma o
 *           valor do nó, e o problema vira remover o sucessor, que por
 *           construção cai num dos dois casos anteriores
 *
 * O terceiro é o que a aula gasta o quadro inteiro explicando. Ele reduz ao
 * caso anterior, e é essa redução que a instrumentação mostra: a descida até
 * o sucessor acontece na tela, nó por nó.                                 */

/* Liga `filho` no lugar que `velho` ocupava em `pai`. */
static void religar(Abb *a, No *pai, const No *velho, No *filho)
{
    if (pai == NULL) {
        a->raiz = filho;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(filho));
    } else if (pai->esq == velho) {
        pai->esq = filho;
        TR(EV_EDGE_SET, .a = id_de(pai), .b = SLOT_ESQ, .c = id_de(filho));
    } else {
        pai->dir = filho;
        TR(EV_EDGE_SET, .a = id_de(pai), .b = SLOT_DIR, .c = id_de(filho));
    }
}

int abb_remover(Abb *a, elem_t valor)
{
    No *atual = a->raiz;
    No *pai = NULL;

    /* Primeira descida: achar o nó. */
    while (atual != NULL && atual->valor != valor) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        TR(EV_MSG, .a = (valor < atual->valor) ? STR_VAI_ESQ : STR_VAI_DIR);
        TR(EV_UNVISIT, .a = id_de(atual));

        pai = atual;
        atual = (valor < atual->valor) ? atual->esq : atual->dir;
    }

    if (atual == NULL) {
        TR(EV_MSG, .a = STR_NAO_ACHOU);
        return ERR_NAO_ENCONTRADO;
    }

    TR(EV_VISIT, .a = id_de(atual));
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
    TR(EV_MSG, .a = STR_ACHOU);

    /* Caso 3: dois filhos. Reduz aos outros dois e continua. */
    if (atual->esq != NULL && atual->dir != NULL) {
        No *sucessor = atual->dir;
        No *pai_sucessor = atual;

        TR(EV_MSG, .a = STR_CASO_DOIS_FILHOS);
        TR(EV_MSG, .a = STR_PROCURA_SUCESSOR);

        /* O menor da subárvore direita: entra à direita uma vez, e depois é
         * sempre à esquerda. Ele não tem filho esquerdo — se tivesse, o filho
         * seria menor que ele, e ele não seria o menor. */
        while (sucessor->esq != NULL) {
            TR(EV_VISIT, .a = id_de(sucessor));
            TR(EV_MSG, .a = STR_VAI_ESQ);
            TR(EV_UNVISIT, .a = id_de(sucessor));
            pai_sucessor = sucessor;
            sucessor = sucessor->esq;
        }

        TR(EV_VISIT, .a = id_de(sucessor));
        TR(EV_MSG, .a = STR_SUBSTITUI);

        /* O valor do sucessor sobe para o nó; a árvore continua ordenada,
         * porque ele é o menor dos maiores. O que sobra para remover é o
         * sucessor, e ele tem no máximo um filho — o direito. */
        atual->valor = sucessor->valor;
        TR(EV_NODE_SET, .a = id_de(atual), .b = 0, .c = sucessor->valor);
        TR(EV_UNVISIT, .a = id_de(atual));

        atual = sucessor;
        pai = pai_sucessor;
    }

    /* Agora `atual` tem no máximo um filho: os casos 1 e 2 juntos, porque a
     * folha é o caso do filho único com o filho valendo NULL. */
    {
        No *filho = (atual->esq != NULL) ? atual->esq : atual->dir;

        TR(EV_MSG, .a = (filho == NULL) ? STR_CASO_FOLHA : STR_CASO_UM_FILHO);
        religar(a, pai, atual, filho);

        TR(EV_UNVISIT, .a = id_de(atual));
        TR(EV_NODE_FREE, .a = id_de(atual));
        id_esquece(atual);
        free(atual);
        TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
    }

    a->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    return OK;
}

/* O menor é o nó mais à esquerda, e chegar nele é descer sempre para o mesmo
 * lado. É a operação que a fila de prioridade faria. */
static No *descer_ate_o_menor(No *raiz, No **pai_saida)
{
    No *atual = raiz;
    No *pai = NULL;

    while (atual != NULL && atual->esq != NULL) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_MSG, .a = STR_VAI_ESQ);
        TR(EV_UNVISIT, .a = id_de(atual));
        pai = atual;
        atual = atual->esq;
    }

    *pai_saida = pai;
    return atual;
}

int abb_menor(const Abb *a, elem_t *saida)
{
    No *pai = NULL;
    No *menor;

    if (a->raiz == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    menor = descer_ate_o_menor(a->raiz, &pai);
    TR(EV_VISIT, .a = id_de(menor));
    *saida = menor->valor;
    return OK;
}

int abb_remover_menor(Abb *a, elem_t *saida)
{
    No *pai = NULL;
    No *menor;

    if (a->raiz == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    menor = descer_ate_o_menor(a->raiz, &pai);
    *saida = menor->valor;

    /* O menor não tem filho esquerdo, por definição: só o direito pode subir. */
    TR(EV_VISIT, .a = id_de(menor));
    religar(a, pai, menor, menor->dir);
    TR(EV_UNVISIT, .a = id_de(menor));
    TR(EV_NODE_FREE, .a = id_de(menor));
    id_esquece(menor);
    free(menor);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);

    a->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    return OK;
}

/* ---- percursos ---------------------------------------------------------
 *
 * Três funções de quatro linhas, e a diferença entre elas é a POSIÇÃO de uma
 * linha. É o exemplo mais curto que existe de recursão mudando de sentido, e
 * na tela a diferença aparece inteira: em ordem sai crescente, pré-ordem sai
 * na ordem em que os nós seriam inseridos para reconstruir a árvore igual, e
 * pós-ordem é a ordem em que dá para liberar sem deixar ponteiro solto.     */

static void visitar(const No *n)
{
    TR(EV_VISIT, .a = id_de(n));
    TR(EV_UNVISIT, .a = id_de(n));
}

static void em_ordem(const No *n)
{
    if (n == NULL) return;
    em_ordem(n->esq);
    visitar(n);
    em_ordem(n->dir);
}

static void pre_ordem(const No *n)
{
    if (n == NULL) return;
    visitar(n);
    pre_ordem(n->esq);
    pre_ordem(n->dir);
}

static void pos_ordem(const No *n)
{
    if (n == NULL) return;
    pos_ordem(n->esq);
    pos_ordem(n->dir);
    visitar(n);
}

int abb_percurso(const Abb *a, int ordem)
{
    TR(EV_MSG, .a = STR_PERCURSO);

    switch (ordem) {
    case PERC_EM_ORDEM:  em_ordem(a->raiz);  return OK;
    case PERC_PRE_ORDEM: pre_ordem(a->raiz); return OK;
    case PERC_POS_ORDEM: pos_ordem(a->raiz); return OK;
    default:             return ERR_ARG_INVALIDO;
    }
}

/* ---- o resto ------------------------------------------------------------ */

/* Libera em pós-ordem, que é a única ordem em que dá para liberar sem tocar
 * num ponteiro já liberado. */
static void liberar(No *n)
{
    if (n == NULL) return;
    liberar(n->esq);
    liberar(n->dir);
    TR(EV_NODE_FREE, .a = id_de(n));
    id_esquece(n);
    free(n);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
}

void abb_limpar(Abb *a)
{
    int quantos = a->n;

    liberar(a->raiz);
    a->raiz = NULL;
    a->n = 0;

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    if (quantos > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -quantos);
    }
}

int abb_tamanho(const Abb *a)
{
    return a->n;
}

static int altura_de(const No *n)
{
    int e;
    int d;

    if (n == NULL) {
        return 0;
    }
    e = altura_de(n->esq);
    d = altura_de(n->dir);
    return 1 + ((e > d) ? e : d);
}

int abb_altura(const Abb *a)
{
    return altura_de(a->raiz);
}

void abb_destruir(Abb *a)
{
    if (a == NULL) {
        return;
    }
    /* Sem trace: destruir é a sessão acabando, e não uma operação para
     * assistir. Emitir aqui encheria o trace da sessão seguinte. */
    trace_set_enabled(0);
    liberar(a->raiz);
    trace_set_enabled(1);
    free(a);
}

/* ---- invariantes ------------------------------------------------------- */

static int copiar_em_ordem(const No *n, elem_t *saida, int max, int *k)
{
    if (n == NULL) {
        return 1;
    }
    if (!copiar_em_ordem(n->esq, saida, max, k)) {
        return 0;
    }
    if (*k >= max) {
        return 0;
    }
    saida[(*k)++] = n->valor;
    return copiar_em_ordem(n->dir, saida, max, k);
}

int abb_em_ordem(const Abb *a, elem_t *saida, int max)
{
    int k = 0;

    trace_set_enabled(0);
    copiar_em_ordem(a->raiz, saida, max, &k);
    trace_set_enabled(1);
    return k;
}

/* A invariante que define a estrutura. Verificar a ordem local — filho
 * esquerdo menor que o pai — não bastaria: um nó pode ser menor que o pai e
 * maior que o avô, e a árvore estaria errada com todos os pares locais certos.
 * O percurso em ordem pega isso. */
int abb_ordenada(const Abb *a)
{
    enum { LIMITE = 4096 };
    static elem_t valores[LIMITE];
    int k;
    int i;

    k = abb_em_ordem(a, valores, LIMITE);
    for (i = 1; i < k; i++) {
        if (valores[i - 1] >= valores[i]) {
            return 0;
        }
    }
    return 1;
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    (void) capacidade;   /* a árvore não tem limite */
    return abb_criar();
}

static void vt_destruir(void *s)
{
    abb_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return abb_inserir(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return abb_remover_menor(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return abb_menor(s, saida);
}

static void vt_limpar(void *s)
{
    abb_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return abb_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return abb_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return abb_remover(s, valor);
}

static int vt_percurso(const void *s, int ordem)
{
    return abb_percurso(s, ordem);
}

const TAD_Linear ABB = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .remover = vt_remover,
    .consultar = vt_consultar,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar,
    .remover_valor = vt_remover_valor,
    .percurso = vt_percurso,
};
