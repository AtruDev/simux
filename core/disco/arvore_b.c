/* core/disco/arvore_b.c — árvore B, com divisão na descida e fusão na volta.
 *
 * Cada nó é uma PÁGINA DE DISCO, e é essa equivalência que dá sentido a tudo
 * o que este arquivo faz. Ler um nó é `LER_PAGINA`, e é a operação cara;
 * comparar as chaves dentro de um nó já lido é de graça. Toda decisão de
 * projeto da árvore B sai dessa razão.
 *
 * A instrumentação segue a mesma regra de sempre — as chamadas TR contam o
 * que aconteceu, e tirá-las devolve o algoritmo de sempre. A diferença é que
 * aqui elas contam duas coisas de uma vez: o que a estrutura fez, e quanto
 * isso custou em páginas.
 *
 * `publicar` reanuncia o nó inteiro depois de cada mudança, em vez de emitir
 * só o campo que mudou. São O(t) eventos por mudança, e valem: um nó de
 * árvore B muda muito de uma vez numa divisão ou numa fusão, e emitir campo a
 * campo seria uma dúzia de lugares para esquecer um. */

#define TR_SRC SRC_ARVORE_B

#include "ds/arvore_b.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/paginador.h"
#include "ds/trace.h"

#include "acessos.h"
#include "linear.h"

enum { CHAVES_MAX = 2 * ARVORE_B_T_MAX - 1 };
enum { FILHOS_MAX = 2 * ARVORE_B_T_MAX };

typedef struct NoB {
    elem_t       chaves[CHAVES_MAX];
    struct NoB  *filhos[FILHOS_MAX];
    int          n;         /* quantas chaves o nó tem agora */
    int          folha;
    int          pagina;
} NoB;

struct ArvoreB {
    NoB      *raiz;
    int       t;
    int       total;
    Paginador pag;
};

/* ---- o nó, e como ele aparece na tela ---------------------------------- */

/* Reanuncia o nó inteiro: quantas chaves, quais, e para quem ele aponta.
 *
 * Chamado depois de toda mudança estrutural. É mais evento do que o mínimo, e
 * é o que garante que o desenho nunca fique meio atualizado — numa divisão o
 * nó perde metade das chaves e ganha um filho novo ao mesmo tempo. */
static void publicar(const NoB *n)
{
    int i;

    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_N, .c = n->n);
    for (i = 0; i < n->n; i++) {
        TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_CHAVE + i, .c = n->chaves[i]);
    }
    for (i = 0; i <= n->n; i++) {
        TR(EV_EDGE_SET, .a = id_de(n), .b = i,
           .c = n->folha ? 0 : id_de(n->filhos[i]));
    }
}

static NoB *no_novo(ArvoreB *a, int folha)
{
    NoB *n = malloc(sizeof *n);
    int  i;

    if (n == NULL) {
        return NULL;
    }

    n->n = 0;
    n->folha = folha;
    n->pagina = paginador_alocar(&a->pag);
    for (i = 0; i < FILHOS_MAX; i++) {
        n->filhos[i] = NULL;
    }

    TR(EV_NODE_NEW, .a = id_de(n), .b = 0);
    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_PAGINA, .c = n->pagina);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);
    publicar(n);

    /* Página nova é página escrita: ela precisa existir no disco antes de
     * alguém apontar para ela. */
    ESCREVER_PAGINA(&a->pag, n->pagina);

    return n;
}

static void no_liberar(ArvoreB *a, NoB *n)
{
    (void) a;
    TR(EV_NODE_FREE, .a = id_de(n));
    id_esquece(n);
    free(n);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
}

/* ---- busca --------------------------------------------------------------
 *
 * Desce um nível por vez, e cada nível custa UMA leitura de página. Dentro do
 * nó, a varredura das chaves é de graça — e é por isso que vale a pena ter
 * muitas chaves por nó.                                                    */

ArvoreB *arvore_b_criar(int t)
{
    ArvoreB *a;

    if (t < 2 || t > ARVORE_B_T_MAX) {
        return NULL;
    }

    a = malloc(sizeof *a);
    if (a == NULL) {
        return NULL;
    }

    a->t = t;
    a->total = 0;
    a->raiz = NULL;
    paginador_iniciar(&a->pag);

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    return a;
}

int arvore_b_buscar(const ArvoreB *a, elem_t chave, int *nivel)
{
    const NoB *n = a->raiz;
    int        profundidade = 0;

    while (n != NULL) {
        int i = 0;

        /* A página inteira vem para a memória de uma vez. Daqui para baixo,
         * tudo o que acontece dentro dela é de graça. */
        LER_PAGINA((Paginador *) &a->pag, n->pagina);
        TR(EV_VISIT, .a = id_de(n));

        while (i < n->n && chave > n->chaves[i]) {
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            i++;
        }
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (i < n->n && chave == n->chaves[i]) {
            TR(EV_MSG, .a = STR_ACHOU);
            *nivel = profundidade;
            return OK;
        }

        TR(EV_UNVISIT, .a = id_de(n));
        if (n->folha) {
            break;
        }
        n = n->filhos[i];
        profundidade++;
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

/* ---- inserção -----------------------------------------------------------
 *
 * Divide na DESCIDA: todo nó cheio por onde se passa é dividido antes de
 * entrar nele. Assim o pai sempre tem espaço para a chave que sobe, e a
 * inserção inteira é uma passada só — em disco, isso quer dizer não ter que
 * reler os pais na volta.                                                  */

/* Divide o filho `i` de `pai`, que está cheio.
 *
 * A chave do MEIO sobe para o pai, e as duas metades viram dois nós. É esta
 * função que faz a árvore crescer, e é a única forma de a altura aumentar:
 * quando a divisão chega à raiz, a árvore ganha um nível inteiro de uma vez —
 * em todos os ramos ao mesmo tempo. É isso que mantém as folhas todas na
 * mesma profundidade. */
static int dividir_filho(ArvoreB *a, NoB *pai, int i)
{
    NoB *cheio = pai->filhos[i];
    NoB *novo = no_novo(a, cheio->folha);
    int  t = a->t;
    int  j;

    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }

    TR(EV_VISIT, .a = id_de(cheio));
    TR(EV_MSG, .a = STR_PAGINA_CHEIA);
    TR(EV_MSG, .a = STR_DIVIDE);

    /* A metade de cima vai para o nó novo. */
    novo->n = t - 1;
    for (j = 0; j < t - 1; j++) {
        novo->chaves[j] = cheio->chaves[j + t];
    }
    if (!cheio->folha) {
        for (j = 0; j < t; j++) {
            novo->filhos[j] = cheio->filhos[j + t];
        }
    }
    cheio->n = t - 1;

    /* Abre espaço no pai para o filho novo e para a chave que sobe. */
    for (j = pai->n; j > i; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    pai->filhos[i + 1] = novo;

    for (j = pai->n - 1; j >= i; j--) {
        pai->chaves[j + 1] = pai->chaves[j];
    }

    TR(EV_MSG, .a = STR_SOBE_CHAVE);
    pai->chaves[i] = cheio->chaves[t - 1];
    pai->n++;

    publicar(cheio);
    publicar(novo);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, cheio->pagina);
    ESCREVER_PAGINA(&a->pag, novo->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);

    TR(EV_UNVISIT, .a = id_de(cheio));
    return OK;
}

/* Insere num nó que sabidamente NÃO está cheio. É a garantia que a divisão
 * na descida oferece, e é o que permite esta função não se preocupar com
 * estouro. */
static int inserir_nao_cheio(ArvoreB *a, NoB *n, elem_t chave)
{
    int i = n->n - 1;

    LER_PAGINA(&a->pag, n->pagina);
    TR(EV_VISIT, .a = id_de(n));

    if (n->folha) {
        while (i >= 0 && n->chaves[i] > chave) {
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            n->chaves[i + 1] = n->chaves[i];
            i--;
        }
        n->chaves[i + 1] = chave;
        n->n++;

        publicar(n);
        ESCREVER_PAGINA(&a->pag, n->pagina);
        TR(EV_UNVISIT, .a = id_de(n));
        return OK;
    }

    while (i >= 0 && n->chaves[i] > chave) {
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        i--;
    }
    i++;

    TR(EV_UNVISIT, .a = id_de(n));

    /* A divisão preventiva: se o filho por onde vamos passar está cheio, ele
     * é dividido AGORA, com o pai ainda em mãos e com espaço garantido. */
    if (n->filhos[i]->n == 2 * a->t - 1) {
        int rc = dividir_filho(a, n, i);

        if (rc != OK) {
            return rc;
        }
        /* A chave do meio subiu para a posição i: se a nova chave é maior que
         * ela, o lugar dela é no filho da direita. */
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        if (chave > n->chaves[i]) {
            i++;
        }
    }

    return inserir_nao_cheio(a, n->filhos[i], chave);
}

int arvore_b_inserir(ArvoreB *a, elem_t chave)
{
    int nivel = -1;

    /* Repetida não entra, pelo mesmo motivo da ABB: sem critério de
     * desempate, ela tornaria a remoção ambígua. */
    if (arvore_b_buscar(a, chave, &nivel) == OK) {
        TR(EV_MSG, .a = STR_JA_EXISTE);
        return OK;
    }

    if (a->raiz == NULL) {
        a->raiz = no_novo(a, 1);
        if (a->raiz == NULL) {
            return ERR_SEM_MEMORIA;
        }
        a->raiz->chaves[0] = chave;
        a->raiz->n = 1;
        publicar(a->raiz);
        ESCREVER_PAGINA(&a->pag, a->raiz->pagina);
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(a->raiz));
        a->total++;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
        return OK;
    }

    /* A raiz cheia é o único caso em que a árvore cresce em ALTURA, e é o
     * único em que a raiz muda. A raiz nova nasce com uma chave só — é a
     * exceção que a árvore B abre, e ela vale só para a raiz. */
    if (a->raiz->n == 2 * a->t - 1) {
        NoB *nova = no_novo(a, 0);
        int  rc;

        if (nova == NULL) {
            return ERR_SEM_MEMORIA;
        }
        nova->filhos[0] = a->raiz;
        rc = dividir_filho(a, nova, 0);
        if (rc != OK) {
            return rc;
        }

        a->raiz = nova;
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(nova));
    }

    if (inserir_nao_cheio(a, a->raiz, chave) != OK) {
        return ERR_SEM_MEMORIA;
    }

    a->total++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    return OK;
}

/* ---- remoção ------------------------------------------------------------
 *
 * A remoção é a divisão ao contrário, e tem uma garantia própria: nunca se
 * desce para um filho com apenas t-1 chaves. Antes de entrar nele, ele ganha
 * uma chave — emprestada de um irmão, ou fundindo-se com um. Assim, quando a
 * chave é finalmente removida, o nó tem folga e a árvore continua válida sem
 * precisar de conserto na volta.
 *
 * O empréstimo é uma ROTAÇÃO pelo pai: a chave do pai desce para o nó que
 * precisa, e a chave do irmão sobe para o lugar dela no pai. A ordem se
 * mantém, e é por isso que funciona.                                       */

static int primeira_maior_ou_igual(const NoB *n, elem_t chave)
{
    int i = 0;

    while (i < n->n && n->chaves[i] < chave) {
        i++;
    }
    return i;
}

static elem_t antecessor(ArvoreB *a, NoB *n, int i)
{
    NoB *atual = n->filhos[i];

    while (!atual->folha) {
        LER_PAGINA(&a->pag, atual->pagina);
        atual = atual->filhos[atual->n];
    }
    LER_PAGINA(&a->pag, atual->pagina);
    return atual->chaves[atual->n - 1];
}

static elem_t sucessor(ArvoreB *a, NoB *n, int i)
{
    NoB *atual = n->filhos[i + 1];

    while (!atual->folha) {
        LER_PAGINA(&a->pag, atual->pagina);
        atual = atual->filhos[0];
    }
    LER_PAGINA(&a->pag, atual->pagina);
    return atual->chaves[0];
}

/* Funde o filho i, a chave i do pai e o filho i+1 num nó só.
 *
 * Acontece quando nenhum dos dois irmãos tem chave sobrando. O resultado tem
 * exatamente 2t-1 chaves — cheio, mas válido —, e o pai perde uma chave. Se o
 * pai era a raiz e ficou sem chave nenhuma, a árvore perde um nível: é a
 * única forma de ela encolher, e o espelho exato de como ela cresce. */
static void fundir(ArvoreB *a, NoB *pai, int i)
{
    NoB *esq = pai->filhos[i];
    NoB *dir = pai->filhos[i + 1];
    int  t = a->t;
    int  j;

    TR(EV_MSG, .a = STR_FUNDE);
    TR(EV_MSG, .a = STR_DESCE_CHAVE);
    TR(EV_VISIT, .a = id_de(esq));
    TR(EV_VISIT, .a = id_de(dir));

    /* A chave do pai desce para o meio: é ela que separa as duas metades, e é
     * exatamente a chave que subiu quando este nó foi dividido. */
    esq->chaves[t - 1] = pai->chaves[i];

    for (j = 0; j < dir->n; j++) {
        esq->chaves[j + t] = dir->chaves[j];
    }
    if (!esq->folha) {
        for (j = 0; j <= dir->n; j++) {
            esq->filhos[j + t] = dir->filhos[j];
        }
    }
    esq->n += dir->n + 1;

    for (j = i + 1; j < pai->n; j++) {
        pai->chaves[j - 1] = pai->chaves[j];
    }
    for (j = i + 2; j <= pai->n; j++) {
        pai->filhos[j - 1] = pai->filhos[j];
    }
    pai->n--;

    TR(EV_UNVISIT, .a = id_de(dir));
    no_liberar(a, dir);

    publicar(esq);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, esq->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);
    TR(EV_UNVISIT, .a = id_de(esq));
}

/* O irmão da esquerda tem chave sobrando: a chave do pai desce para o filho, e
 * a maior chave do irmão sobe para o lugar dela. */
static void emprestar_do_anterior(ArvoreB *a, NoB *pai, int i)
{
    NoB *filho = pai->filhos[i];
    NoB *irmao = pai->filhos[i - 1];
    int  j;

    TR(EV_MSG, .a = STR_EMPRESTA_ESQ);
    TR(EV_VISIT, .a = id_de(irmao));

    for (j = filho->n - 1; j >= 0; j--) {
        filho->chaves[j + 1] = filho->chaves[j];
    }
    if (!filho->folha) {
        for (j = filho->n; j >= 0; j--) {
            filho->filhos[j + 1] = filho->filhos[j];
        }
        filho->filhos[0] = irmao->filhos[irmao->n];
    }

    filho->chaves[0] = pai->chaves[i - 1];
    pai->chaves[i - 1] = irmao->chaves[irmao->n - 1];

    filho->n++;
    irmao->n--;

    publicar(filho);
    publicar(irmao);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, filho->pagina);
    ESCREVER_PAGINA(&a->pag, irmao->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);
    TR(EV_UNVISIT, .a = id_de(irmao));
}

static void emprestar_do_proximo(ArvoreB *a, NoB *pai, int i)
{
    NoB *filho = pai->filhos[i];
    NoB *irmao = pai->filhos[i + 1];
    int  j;

    TR(EV_MSG, .a = STR_EMPRESTA_DIR);
    TR(EV_VISIT, .a = id_de(irmao));

    filho->chaves[filho->n] = pai->chaves[i];
    if (!filho->folha) {
        filho->filhos[filho->n + 1] = irmao->filhos[0];
    }

    pai->chaves[i] = irmao->chaves[0];

    for (j = 1; j < irmao->n; j++) {
        irmao->chaves[j - 1] = irmao->chaves[j];
    }
    if (!irmao->folha) {
        for (j = 1; j <= irmao->n; j++) {
            irmao->filhos[j - 1] = irmao->filhos[j];
        }
    }

    filho->n++;
    irmao->n--;

    publicar(filho);
    publicar(irmao);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, filho->pagina);
    ESCREVER_PAGINA(&a->pag, irmao->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);
    TR(EV_UNVISIT, .a = id_de(irmao));
}

/* Garante que o filho `i` tenha ao menos t chaves antes de se descer nele. */
static void preencher(ArvoreB *a, NoB *pai, int i)
{
    if (i != 0 && pai->filhos[i - 1]->n >= a->t) {
        emprestar_do_anterior(a, pai, i);
    } else if (i != pai->n && pai->filhos[i + 1]->n >= a->t) {
        emprestar_do_proximo(a, pai, i);
    } else if (i != pai->n) {
        fundir(a, pai, i);
    } else {
        fundir(a, pai, i - 1);
    }
}

static int remover_de(ArvoreB *a, NoB *n, elem_t chave);

/* A chave está NESTE nó, e ele é interno. Não dá para simplesmente tirá-la: ela
 * separa dois filhos, e alguém tem que ficar no lugar dela. */
static int remover_interno(ArvoreB *a, NoB *n, int i)
{
    elem_t chave = n->chaves[i];
    int    t = a->t;

    if (n->filhos[i]->n >= t) {
        /* O antecessor sobe: a maior chave da subárvore da esquerda. */
        elem_t pred = antecessor(a, n, i);

        TR(EV_MSG, .a = STR_SUBSTITUI);
        n->chaves[i] = pred;
        publicar(n);
        ESCREVER_PAGINA(&a->pag, n->pagina);
        return remover_de(a, n->filhos[i], pred);
    }

    if (n->filhos[i + 1]->n >= t) {
        elem_t suc = sucessor(a, n, i);

        TR(EV_MSG, .a = STR_SUBSTITUI);
        n->chaves[i] = suc;
        publicar(n);
        ESCREVER_PAGINA(&a->pag, n->pagina);
        return remover_de(a, n->filhos[i + 1], suc);
    }

    /* Nenhum dos dois lados tem chave sobrando: os dois e a chave viram um nó
     * só, e a remoção continua lá dentro. */
    fundir(a, n, i);
    return remover_de(a, n->filhos[i], chave);
}

static int remover_de(ArvoreB *a, NoB *n, elem_t chave)
{
    int i;

    LER_PAGINA(&a->pag, n->pagina);
    TR(EV_VISIT, .a = id_de(n));

    i = primeira_maior_ou_igual(n, chave);
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

    if (i < n->n && n->chaves[i] == chave) {
        if (n->folha) {
            int j;

            for (j = i + 1; j < n->n; j++) {
                n->chaves[j - 1] = n->chaves[j];
            }
            n->n--;
            publicar(n);
            ESCREVER_PAGINA(&a->pag, n->pagina);
            TR(EV_UNVISIT, .a = id_de(n));
            return OK;
        }

        TR(EV_UNVISIT, .a = id_de(n));
        return remover_interno(a, n, i);
    }

    if (n->folha) {
        TR(EV_UNVISIT, .a = id_de(n));
        return ERR_NAO_ENCONTRADO;
    }

    {
        /* `ultimo` guarda se a descida seria pelo último filho: uma fusão pode
         * fazer esse filho desaparecer, e aí o caminho passa a ser o anterior.
         * É o detalhe que quase todo mundo erra na primeira vez. */
        int ultimo = (i == n->n);

        TR(EV_UNVISIT, .a = id_de(n));

        if (n->filhos[i]->n < a->t) {
            preencher(a, n, i);
        }

        if (ultimo && i > n->n) {
            return remover_de(a, n->filhos[i - 1], chave);
        }
        return remover_de(a, n->filhos[i], chave);
    }
}

int arvore_b_remover(ArvoreB *a, elem_t chave)
{
    int rc;

    if (a->raiz == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    rc = remover_de(a, a->raiz, chave);

    /* A raiz sem chave nenhuma é o único jeito de a árvore encolher em altura,
     * e é o espelho exato de como ela cresce.
     *
     * O colapso vem ANTES de olhar o rc, e essa ordem é a correção de um bug
     * que o fuzz achou. Uma remoção que NÃO encontra a chave ainda reestrutura
     * a árvore na descida — `preencher` empresta e funde para garantir que
     * nunca se desça num nó magro, e isso acontece antes de se saber se a
     * chave existe. Uma fusão na raiz a deixa com zero chaves, e sair pelo
     * caminho do erro deixava a árvore num estado que ela mesma considera
     * inválido: uma raiz vazia com dois níveis de filhos abaixo. As chaves
     * continuavam todas encontráveis, e foi só a invariante que denunciou. */
    if (a->raiz != NULL && a->raiz->n == 0) {
        NoB *velha = a->raiz;

        a->raiz = velha->folha ? NULL : velha->filhos[0];
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(a->raiz));
        no_liberar(a, velha);
    }

    if (rc != OK) {
        TR(EV_MSG, .a = STR_NAO_ACHOU);
        return rc;
    }

    a->total--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    return OK;
}

/* ---- o resto ------------------------------------------------------------ */

static void liberar(ArvoreB *a, NoB *n)
{
    int i;

    if (n == NULL) {
        return;
    }
    if (!n->folha) {
        for (i = 0; i <= n->n; i++) {
            liberar(a, n->filhos[i]);
        }
    }
    no_liberar(a, n);
}

void arvore_b_limpar(ArvoreB *a)
{
    int quantos = a->total;

    liberar(a, a->raiz);
    a->raiz = NULL;
    a->total = 0;

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    if (quantos > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -quantos);
    }
}

int arvore_b_tamanho(const ArvoreB *a)
{
    return a->total;
}

int arvore_b_grau(const ArvoreB *a)
{
    return a->t;
}

long arvore_b_leituras(const ArvoreB *a)
{
    return paginador_leituras(&a->pag);
}

long arvore_b_escritas(const ArvoreB *a)
{
    return paginador_escritas(&a->pag);
}

int arvore_b_altura(const ArvoreB *a)
{
    const NoB *n = a->raiz;
    int        h = 0;

    while (n != NULL) {
        h++;
        if (n->folha) {
            break;
        }
        n = n->filhos[0];
    }
    return h;
}

void arvore_b_destruir(ArvoreB *a)
{
    if (a == NULL) {
        return;
    }
    trace_set_enabled(0);
    liberar(a, a->raiz);
    trace_set_enabled(1);
    free(a);
}

/* ---- invariantes ------------------------------------------------------- */

static int contem_em(const NoB *n, elem_t chave)
{
    int i;

    if (n == NULL) {
        return 0;
    }
    i = primeira_maior_ou_igual(n, chave);
    if (i < n->n && n->chaves[i] == chave) {
        return 1;
    }
    return n->folha ? 0 : contem_em(n->filhos[i], chave);
}

int arvore_b_contem(const ArvoreB *a, elem_t chave)
{
    return contem_em(a->raiz, chave);
}

static int copiar_em_ordem(const NoB *n, elem_t *saida, int max, int *k)
{
    int i;

    if (n == NULL) {
        return 1;
    }
    for (i = 0; i < n->n; i++) {
        if (!n->folha && !copiar_em_ordem(n->filhos[i], saida, max, k)) {
            return 0;
        }
        if (*k >= max) {
            return 0;
        }
        saida[(*k)++] = n->chaves[i];
    }
    if (!n->folha) {
        return copiar_em_ordem(n->filhos[n->n], saida, max, k);
    }
    return 1;
}

int arvore_b_em_ordem(const ArvoreB *a, elem_t *saida, int max)
{
    int k = 0;

    trace_set_enabled(0);
    copiar_em_ordem(a->raiz, saida, max, &k);
    trace_set_enabled(1);
    return k;
}

/* Confere as três promessas de uma vez, e devolve a profundidade das folhas
 * (ou -1 se algo estiver errado).
 *
 * `menor` e `maior` são a faixa que o pai delimita para esta subárvore. É a
 * terceira promessa, e a que pega o erro sutil: uma divisão que sobe a chave
 * errada deixa a árvore com a forma certa e a busca quebrada. */
static int conferir(const NoB *n, int t, int eh_raiz, elem_t menor, elem_t maior,
                    int tem_menor, int tem_maior)
{
    int i;
    int profundidade = -1;

    if (n == NULL) {
        return 0;
    }

    if (n->n > 2 * t - 1) {
        return -1;
    }
    if (!eh_raiz && n->n < t - 1) {
        return -1;
    }
    if (eh_raiz && n->n < 1) {
        return -1;
    }

    for (i = 0; i < n->n; i++) {
        if (i > 0 && n->chaves[i - 1] >= n->chaves[i]) {
            return -1;
        }
        if (tem_menor && n->chaves[i] <= menor) {
            return -1;
        }
        if (tem_maior && n->chaves[i] >= maior) {
            return -1;
        }
    }

    if (n->folha) {
        return 1;
    }

    for (i = 0; i <= n->n; i++) {
        int sub = conferir(n->filhos[i], t, 0,
                           (i > 0) ? n->chaves[i - 1] : menor,
                           (i < n->n) ? n->chaves[i] : maior,
                           (i > 0) ? 1 : tem_menor,
                           (i < n->n) ? 1 : tem_maior);

        if (sub < 0) {
            return -1;
        }
        /* Todas as folhas na MESMA profundidade: é a promessa que a divisão
         * pela raiz existe para manter. */
        if (profundidade < 0) {
            profundidade = sub;
        } else if (sub != profundidade) {
            return -1;
        }
    }

    return profundidade + 1;
}

int arvore_b_valida(const ArvoreB *a)
{
    if (a->raiz == NULL) {
        return a->total == 0;
    }
    return conferir(a->raiz, a->t, 1, 0, 0, 0, 0) >= 0;
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * A capacidade da sessão é o GRAU t: é o único parâmetro que a árvore B tem,
 * e é ele que muda a forma inteira dela.                                    */

static void *vt_criar(int capacidade)
{
    return arvore_b_criar(capacidade);
}

static void vt_destruir(void *s)
{
    arvore_b_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return arvore_b_inserir(s, valor);
}

static void vt_limpar(void *s)
{
    arvore_b_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return arvore_b_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return arvore_b_grau(s);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return arvore_b_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return arvore_b_remover(s, valor);
}

const TAD_Linear ARVORE_B = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar,
    .remover_valor = vt_remover_valor,
};
