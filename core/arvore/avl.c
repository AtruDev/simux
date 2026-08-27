/* core/arvore/avl.c — árvore AVL.
 *
 * É a ABB do arquivo ao lado com uma promessa a mais: |FB| <= 1 em todo nó. O
 * fator de balanceamento é a altura da subárvore esquerda menos a da direita,
 * e manter esse número entre -1 e 1 é tudo o que a AVL faz de diferente.
 *
 * A inserção e a remoção são recursivas aqui, e na ABB são iterativas. Não é
 * inconsistência: a AVL precisa VOLTAR pelo caminho da descida, atualizando
 * alturas e rebalanceando, e a recursão é o que dá esse caminho de volta de
 * graça. Escrita iterativa, ela precisaria de uma pilha de pais explícita — e
 * é justamente o código que a aula não escreve.
 *
 * As quatro rotações são duas, na verdade. Esquerda-direita é uma rotação à
 * esquerda no filho seguida de uma à direita no nó; direita-esquerda é a
 * espelhada. O código diz isso literalmente, e é por isso que `rebalancear`
 * cabe numa tela.
 *
 * O FB de cada nó vai para a tela por EV_NODE_SET no campo CAMPO_FB. Sem ele,
 * a rotação parece mágica: o número que estourou é a única explicação de por
 * que ela aconteceu naquele nó e não em outro. */

#define TR_SRC SRC_AVL

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
    int        altura;      /* 1 para folha; guardada, não recalculada */
    struct No *esq;
    struct No *dir;
} No;

struct Avl {
    No *raiz;
    int n;
    int rotacoes;
};

/* ---- alturas e fator de balanceamento ---------------------------------- */

static int altura(const No *n)
{
    return (n == NULL) ? 0 : n->altura;
}

static int maior(int a, int b)
{
    return (a > b) ? a : b;
}

static int fb(const No *n)
{
    return (n == NULL) ? 0 : altura(n->esq) - altura(n->dir);
}

/* Recalcula a altura a partir dos filhos e anuncia o FB.
 *
 * Chamada só em quem pode ter mudado — os nós do caminho de volta da recursão,
 * e os dois de cada rotação. Quem não está nesse caminho não muda de altura, e
 * por isso não aparece no trace: são O(log n) eventos por operação, não O(n).
 *
 * O FB é anunciado sempre, e não só quando muda, porque `reavaliar` não tem
 * como saber o valor anterior sem guardar um campo de visualização dentro do
 * nó — e a struct do nó tem que continuar sendo a da matéria. */
static void reavaliar(No *n)
{
    n->altura = 1 + maior(altura(n->esq), altura(n->dir));
    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_FB, .c = fb(n));
}

static No *no_novo(elem_t valor)
{
    No *n = malloc(sizeof *n);

    if (n == NULL) {
        return NULL;
    }
    n->valor = valor;
    n->altura = 1;
    n->esq = NULL;
    n->dir = NULL;

    TR(EV_NODE_NEW, .a = id_de(n), .b = valor);
    TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_ESQ, .c = 0);
    TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = 0);
    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_FB, .c = 0);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    return n;
}

/* ---- as duas rotações ---------------------------------------------------
 *
 * Rodar à direita em `y`:
 *
 *        y            x
 *       / \          / \
 *      x   c   ->   a   y
 *     / \              / \
 *    a   b            b   c
 *
 * O `b` troca de pai, e é o único ponteiro que não é óbvio. Repare que a
 * ordem se mantém: b estava entre x e y antes, e continua entre x e y depois.
 * É essa observação que prova que a rotação preserva a busca.               */

static No *rodar_dir(Avl *a, No *y)
{
    No *x = y->esq;
    No *b = x->dir;

    TR(EV_VISIT, .a = id_de(y));
    TR(EV_VISIT, .a = id_de(x));

    x->dir = y;
    y->esq = b;

    TR(EV_EDGE_SET, .a = id_de(y), .b = SLOT_ESQ, .c = id_de(b));
    TR(EV_EDGE_SET, .a = id_de(x), .b = SLOT_DIR, .c = id_de(y));

    /* y primeiro: ele virou filho, e a altura de x depende da dele. */
    reavaliar(y);
    reavaliar(x);

    a->rotacoes++;
    TR(EV_COUNT, .a = CNT_ROTACOES, .b = +1);
    TR(EV_UNVISIT, .a = id_de(y));
    TR(EV_UNVISIT, .a = id_de(x));

    return x;
}

static No *rodar_esq(Avl *a, No *x)
{
    No *y = x->dir;
    No *b = y->esq;

    TR(EV_VISIT, .a = id_de(x));
    TR(EV_VISIT, .a = id_de(y));

    y->esq = x;
    x->dir = b;

    TR(EV_EDGE_SET, .a = id_de(x), .b = SLOT_DIR, .c = id_de(b));
    TR(EV_EDGE_SET, .a = id_de(y), .b = SLOT_ESQ, .c = id_de(x));

    reavaliar(x);
    reavaliar(y);

    a->rotacoes++;
    TR(EV_COUNT, .a = CNT_ROTACOES, .b = +1);
    TR(EV_UNVISIT, .a = id_de(x));
    TR(EV_UNVISIT, .a = id_de(y));

    return y;
}

/* Os quatro casos, que são dois espelhados.
 *
 * O nome do caso vem de ONDE o desequilíbrio está: esquerda-esquerda quer
 * dizer "pesado à esquerda, e o filho esquerdo também pesa à esquerda". Nesse
 * caso uma rotação basta. Quando os dois lados discordam — esquerda-direita —,
 * a primeira rotação alinha o filho e a segunda resolve.
 *
 * Devolve a nova raiz da subárvore. */
static No *rebalancear(Avl *a, No *n)
{
    int equilibrio;

    reavaliar(n);
    equilibrio = fb(n);

    if (equilibrio > 1) {
        TR(EV_MSG, .a = STR_DESBALANCEOU);

        if (fb(n->esq) < 0) {
            /* esquerda-direita: alinha o filho antes */
            TR(EV_MSG, .a = STR_ROT_ESQ_DIR);
            n->esq = rodar_esq(a, n->esq);
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_ESQ, .c = id_de(n->esq));
        } else {
            TR(EV_MSG, .a = STR_ROT_DIR);
        }

        n = rodar_dir(a, n);
        TR(EV_MSG, .a = STR_REEQUILIBRADA);
        return n;
    }

    if (equilibrio < -1) {
        TR(EV_MSG, .a = STR_DESBALANCEOU);

        if (fb(n->dir) > 0) {
            TR(EV_MSG, .a = STR_ROT_DIR_ESQ);
            n->dir = rodar_dir(a, n->dir);
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = id_de(n->dir));
        } else {
            TR(EV_MSG, .a = STR_ROT_ESQ);
        }

        n = rodar_esq(a, n);
        TR(EV_MSG, .a = STR_REEQUILIBRADA);
        return n;
    }

    return n;
}

/* ---- inserção ----------------------------------------------------------- */

Avl *avl_criar(void)
{
    Avl *a = malloc(sizeof *a);

    if (a != NULL) {
        a->raiz = NULL;
        a->n = 0;
        a->rotacoes = 0;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    }
    return a;
}

/* Desce, insere na folha, e rebalanceia na volta.
 *
 * `entrou` diz se um nó foi de fato criado: repetido não entra, e nesse caso
 * não há nada para rebalancear na volta. */
static No *inserir_em(Avl *a, No *n, elem_t valor, int *entrou)
{
    No *antes;

    if (n == NULL) {
        No *novo = no_novo(valor);

        *entrou = (novo != NULL);
        return novo;
    }

    TR(EV_VISIT, .a = id_de(n));
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

    if (valor == n->valor) {
        TR(EV_MSG, .a = STR_JA_EXISTE);
        TR(EV_UNVISIT, .a = id_de(n));
        return n;
    }

    if (valor < n->valor) {
        TR(EV_MSG, .a = STR_VAI_ESQ);
        TR(EV_UNVISIT, .a = id_de(n));
        antes = n->esq;
        n->esq = inserir_em(a, n->esq, valor, entrou);
        if (n->esq != antes) {
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_ESQ, .c = id_de(n->esq));
        }
    } else {
        TR(EV_MSG, .a = STR_VAI_DIR);
        TR(EV_UNVISIT, .a = id_de(n));
        antes = n->dir;
        n->dir = inserir_em(a, n->dir, valor, entrou);
        if (n->dir != antes) {
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = id_de(n->dir));
        }
    }

    /* Só quem está no caminho de uma inserção que aconteceu pode ter mudado
     * de altura. Rebalancear na volta de um repetido seria trabalho sobre uma
     * árvore que ninguém mexeu. */
    if (!*entrou) {
        return n;
    }
    return rebalancear(a, n);
}

int avl_inserir(Avl *a, elem_t valor)
{
    int entrou = 0;
    No *raiz = inserir_em(a, a->raiz, valor, &entrou);

    if (raiz == NULL) {
        return ERR_SEM_MEMORIA;
    }

    if (raiz != a->raiz) {
        a->raiz = raiz;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(raiz));
    }

    if (entrou) {
        a->n++;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    }
    return OK;
}

/* ---- remoção ------------------------------------------------------------
 *
 * Os mesmos três casos da ABB, e um quarto passo que a ABB não tem: na volta
 * da recursão, cada nó do caminho é rebalanceado. Uma remoção pode desbalancear
 * vários níveis de uma vez — diferente da inserção, em que uma rotação sempre
 * basta —, e é por isso que a chamada a `rebalancear` fica no caminho de volta
 * inteiro, e não só onde o nó saiu.                                        */

static No *menor_de(No *n)
{
    while (n->esq != NULL) {
        TR(EV_VISIT, .a = id_de(n));
        TR(EV_MSG, .a = STR_VAI_ESQ);
        TR(EV_UNVISIT, .a = id_de(n));
        n = n->esq;
    }
    return n;
}

static No *remover_de(Avl *a, No *n, elem_t valor, int *saiu)
{
    No *antes;

    if (n == NULL) {
        return NULL;
    }

    TR(EV_VISIT, .a = id_de(n));
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

    if (valor < n->valor) {
        TR(EV_MSG, .a = STR_VAI_ESQ);
        TR(EV_UNVISIT, .a = id_de(n));
        antes = n->esq;
        n->esq = remover_de(a, n->esq, valor, saiu);
        if (n->esq != antes) {
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_ESQ, .c = id_de(n->esq));
        }
    } else if (valor > n->valor) {
        TR(EV_MSG, .a = STR_VAI_DIR);
        TR(EV_UNVISIT, .a = id_de(n));
        antes = n->dir;
        n->dir = remover_de(a, n->dir, valor, saiu);
        if (n->dir != antes) {
            TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = id_de(n->dir));
        }
    } else {
        TR(EV_MSG, .a = STR_ACHOU);
        *saiu = 1;

        if (n->esq == NULL || n->dir == NULL) {
            /* Casos 1 e 2 juntos: a folha é o filho único com o filho valendo
             * NULL. Quem sobra toma o lugar, e o nó sai. */
            No *filho = (n->esq != NULL) ? n->esq : n->dir;

            TR(EV_MSG, .a = (filho == NULL) ? STR_CASO_FOLHA : STR_CASO_UM_FILHO);
            TR(EV_UNVISIT, .a = id_de(n));
            TR(EV_NODE_FREE, .a = id_de(n));
            id_esquece(n);
            free(n);
            TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
            return filho;
        }

        /* Caso 3: o sucessor em ordem sobe, e o problema vira removê-lo. */
        {
            No *sucessor;

            TR(EV_MSG, .a = STR_CASO_DOIS_FILHOS);
            TR(EV_MSG, .a = STR_PROCURA_SUCESSOR);
            sucessor = menor_de(n->dir);

            TR(EV_VISIT, .a = id_de(sucessor));
            TR(EV_MSG, .a = STR_SUBSTITUI);
            n->valor = sucessor->valor;
            TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_VALOR, .c = n->valor);
            TR(EV_UNVISIT, .a = id_de(sucessor));

            antes = n->dir;
            n->dir = remover_de(a, n->dir, sucessor->valor, saiu);
            if (n->dir != antes) {
                TR(EV_EDGE_SET, .a = id_de(n), .b = SLOT_DIR, .c = id_de(n->dir));
            }
            TR(EV_UNVISIT, .a = id_de(n));
        }
    }

    return rebalancear(a, n);
}

int avl_remover(Avl *a, elem_t valor)
{
    int saiu = 0;
    No *raiz = remover_de(a, a->raiz, valor, &saiu);

    if (raiz != a->raiz) {
        a->raiz = raiz;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(raiz));
    }

    if (!saiu) {
        TR(EV_MSG, .a = STR_NAO_ACHOU);
        return ERR_NAO_ENCONTRADO;
    }

    a->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    return OK;
}

/* ---- o resto, que é igual ao da ABB ------------------------------------- */

int avl_menor(const Avl *a, elem_t *saida)
{
    const No *n = a->raiz;

    if (n == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }
    while (n->esq != NULL) {
        TR(EV_VISIT, .a = id_de(n));
        TR(EV_MSG, .a = STR_VAI_ESQ);
        TR(EV_UNVISIT, .a = id_de(n));
        n = n->esq;
    }
    TR(EV_VISIT, .a = id_de(n));
    *saida = n->valor;
    return OK;
}

int avl_remover_menor(Avl *a, elem_t *saida)
{
    int rc = avl_menor(a, saida);

    if (rc != OK) {
        return rc;
    }
    return avl_remover(a, *saida);
}

int avl_buscar(const Avl *a, elem_t valor, int *profundidade)
{
    const No *n = a->raiz;
    int       nivel = 0;

    while (n != NULL) {
        TR(EV_VISIT, .a = id_de(n));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (valor == n->valor) {
            TR(EV_MSG, .a = STR_ACHOU);
            *profundidade = nivel;
            return OK;
        }

        TR(EV_MSG, .a = (valor < n->valor) ? STR_VAI_ESQ : STR_VAI_DIR);
        TR(EV_UNVISIT, .a = id_de(n));
        n = (valor < n->valor) ? n->esq : n->dir;
        nivel++;
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

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

int avl_percurso(const Avl *a, int ordem)
{
    TR(EV_MSG, .a = STR_PERCURSO);

    switch (ordem) {
    case PERC_EM_ORDEM:  em_ordem(a->raiz);  return OK;
    case PERC_PRE_ORDEM: pre_ordem(a->raiz); return OK;
    case PERC_POS_ORDEM: pos_ordem(a->raiz); return OK;
    default:             return ERR_ARG_INVALIDO;
    }
}

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

void avl_limpar(Avl *a)
{
    int quantos = a->n;

    liberar(a->raiz);
    a->raiz = NULL;
    a->n = 0;

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    if (quantos > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -quantos);
    }
    if (a->rotacoes > 0) {
        TR(EV_COUNT, .a = CNT_ROTACOES, .b = -a->rotacoes);
        a->rotacoes = 0;
    }
}

int avl_tamanho(const Avl *a)
{
    return a->n;
}

int avl_altura(const Avl *a)
{
    return altura(a->raiz);
}

int avl_rotacoes(const Avl *a)
{
    return a->rotacoes;
}

void avl_destruir(Avl *a)
{
    if (a == NULL) {
        return;
    }
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

int avl_em_ordem(const Avl *a, elem_t *saida, int max)
{
    int k = 0;

    trace_set_enabled(0);
    copiar_em_ordem(a->raiz, saida, max, &k);
    trace_set_enabled(1);
    return k;
}

int avl_ordenada(const Avl *a)
{
    enum { LIMITE = 4096 };
    static elem_t valores[LIMITE];
    int k;
    int i;

    k = avl_em_ordem(a, valores, LIMITE);
    for (i = 1; i < k; i++) {
        if (valores[i - 1] >= valores[i]) {
            return 0;
        }
    }
    return 1;
}

/* Devolve a altura real, ou -1 se a subárvore violar alguma coisa.
 *
 * Duas violações, e as duas importam. |FB| > 1 é a promessa quebrada. A altura
 * guardada não bater com a real é pior: a árvore fica equilibrada de fato e
 * mentindo sobre si mesma, e o desequilíbrio aparece dezenas de inserções
 * depois, longe da rotação que esqueceu de atualizar. */
static int conferir_no(const No *n)
{
    int e;
    int d;

    if (n == NULL) {
        return 0;
    }

    e = conferir_no(n->esq);
    d = conferir_no(n->dir);
    if (e < 0 || d < 0) {
        return -1;
    }

    if (e - d > 1 || d - e > 1) {
        return -1;
    }
    if (n->altura != 1 + maior(e, d)) {
        return -1;
    }
    return n->altura;
}

int avl_equilibrada(const Avl *a)
{
    return conferir_no(a->raiz) >= 0;
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    (void) capacidade;
    return avl_criar();
}

static void vt_destruir(void *s)
{
    avl_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return avl_inserir(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return avl_remover_menor(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return avl_menor(s, saida);
}

static void vt_limpar(void *s)
{
    avl_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return avl_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return avl_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return avl_remover(s, valor);
}

static int vt_percurso(const void *s, int ordem)
{
    return avl_percurso(s, ordem);
}

const TAD_Linear AVL = {
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
