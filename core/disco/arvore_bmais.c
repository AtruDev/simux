/* core/disco/arvore_bmais.c — árvore B+: os dados nas folhas, e as folhas
 * encadeadas.
 *
 * Este arquivo é o irmão de arvore_b.c, e vale ler os dois lado a lado: a
 * forma é a mesma — divisão na descida, fusão na volta, todas as folhas na
 * mesma profundidade — e o que muda são três pontos, cada um com uma linha
 * de código e uma consequência grande:
 *
 *   dividir_filho  a chave do meio de uma FOLHA é copiada, não movida: ela
 *                  continua embaixo, porque é lá que os dados moram;
 *   fundir         a chave do pai NÃO desce quando os filhos são folhas: ela
 *                  era só separador, e some sem ninguém perder dado;
 *   buscar         desce sempre até a folha, mesmo achando o valor no meio do
 *                  caminho — no meio do caminho não há dado, há roteiro.
 *
 * E um quarto ponto que não existe na árvore B: o ELO. Toda folha aponta para
 * a folha seguinte, e a árvore guarda a primeira. É o que faz varrer em ordem
 * custar uma leitura por folha, contra o sobe-e-desce da árvore B — e é a
 * razão de praticamente todo índice de banco de dados ser B+.
 *
 * Não é implementada em termos da árvore B de propósito. Fatorar as partes
 * iguais deixaria um arquivo com `if (eh_bmais)` em seis lugares, e é
 * exatamente nesses seis lugares que a lição está. Duas leituras curtas
 * ensinam mais que uma leitura com desvios. */

#define TR_SRC SRC_ARVORE_B_MAIS

#include "ds/arvore_bmais.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/paginador.h"
#include "ds/trace.h"

#include "acessos.h"
#include "linear.h"

enum { CHAVES_MAX = 2 * ARVORE_BMAIS_T_MAX - 1 };
enum { FILHOS_MAX = 2 * ARVORE_BMAIS_T_MAX };

typedef struct NoBM {
    elem_t        chaves[CHAVES_MAX];
    struct NoBM  *filhos[FILHOS_MAX];
    /* Só a folha usa. É o ponteiro que sobra numa folha — ela não tem filho —
     * e é literalmente assim que a estrutura é implementada em disco. */
    struct NoBM  *proxima;
    int           n;
    int           folha;
    int           pagina;
} NoBM;

struct ArvoreBMais {
    NoBM     *raiz;
    /* A folha mais à esquerda: onde a varredura começa, e o único ponteiro
     * que a árvore B não tem. */
    NoBM     *primeira;
    int       t;
    int       total;
    Paginador pag;
};

/* ---- o nó, e como ele aparece na tela ---------------------------------- */

/* Reanuncia o nó inteiro, como na árvore B, com uma diferença: o frontend
 * precisa saber se o nó é FOLHA, porque isso muda o que os ponteiros dele
 * querem dizer. Num nó interno, os slots são filhos; numa folha, o slot 0 é o
 * elo para a folha seguinte — o mesmo ponteiro que a página tem sobrando. */
static void publicar(const NoBM *n)
{
    int i;

    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_N, .c = n->n);
    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_FOLHA, .c = n->folha);
    for (i = 0; i < n->n; i++) {
        TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_CHAVE + i, .c = n->chaves[i]);
    }

    if (n->folha) {
        TR(EV_EDGE_SET, .a = id_de(n), .b = 0, .c = id_de(n->proxima));
        return;
    }
    for (i = 0; i <= n->n; i++) {
        TR(EV_EDGE_SET, .a = id_de(n), .b = i, .c = id_de(n->filhos[i]));
    }
}

static NoBM *no_novo(ArvoreBMais *a, int folha)
{
    NoBM *n = malloc(sizeof *n);
    int   i;

    if (n == NULL) {
        return NULL;
    }

    n->n = 0;
    n->folha = folha;
    n->proxima = NULL;
    n->pagina = paginador_alocar(&a->pag);
    for (i = 0; i < FILHOS_MAX; i++) {
        n->filhos[i] = NULL;
    }

    TR(EV_NODE_NEW, .a = id_de(n), .b = 0);
    TR(EV_NODE_SET, .a = id_de(n), .b = CAMPO_PAGINA, .c = n->pagina);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);
    publicar(n);

    ESCREVER_PAGINA(&a->pag, n->pagina);
    return n;
}

static void no_liberar(NoBM *n)
{
    TR(EV_NODE_FREE, .a = id_de(n));
    id_esquece(n);
    free(n);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
}

/* ---- roteamento ---------------------------------------------------------
 *
 * A regra de uma B+ inclui o separador à DIREITA: a chave igual ao separador
 * está na subárvore da direita, porque o separador é uma CÓPIA da menor chave
 * de lá. Numa árvore B, em que a chave do meio sobe e some de baixo, a regra
 * é a oposta — e trocar as duas por engano dá uma árvore que perde
 * exatamente as chaves que são iguais a algum separador.                    */

static int rota(const NoBM *n, elem_t chave)
{
    int i = 0;

    while (i < n->n && chave >= n->chaves[i]) {
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        i++;
    }
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
    return i;
}

/* ---- criação e busca ---------------------------------------------------- */

ArvoreBMais *arvore_bmais_criar(int t)
{
    ArvoreBMais *a;

    if (t < 2 || t > ARVORE_BMAIS_T_MAX) {
        return NULL;
    }

    a = malloc(sizeof *a);
    if (a == NULL) {
        return NULL;
    }

    a->t = t;
    a->total = 0;
    a->raiz = NULL;
    a->primeira = NULL;
    paginador_iniciar(&a->pag);

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    TR(EV_PTR_SET, .a = PTR_INICIO, .b = 0);
    return a;
}

/* Desce até a folha, SEMPRE. Passar por cima da chave procurada num nó
 * interno não termina a busca: ali está o separador, não o dado.
 *
 * É o preço da B+, e o custo dele é exato: uma busca lê a altura em páginas,
 * todas as vezes. Na árvore B, uma chave que mora na raiz custa uma. */
int arvore_bmais_buscar(const ArvoreBMais *a, elem_t chave, int *nivel)
{
    const NoBM *n = a->raiz;
    int         profundidade = 0;
    int         i;

    while (n != NULL && !n->folha) {
        LER_PAGINA((Paginador *) &a->pag, n->pagina);
        TR(EV_VISIT, .a = id_de(n));
        i = rota(n, chave);
        TR(EV_UNVISIT, .a = id_de(n));

        n = n->filhos[i];
        profundidade++;
    }

    if (n == NULL) {
        TR(EV_MSG, .a = STR_NAO_ACHOU);
        *nivel = 0;
        return ERR_NAO_ENCONTRADO;
    }

    /* A folha inteira vem numa leitura só, e varrê-la é de graça. */
    LER_PAGINA((Paginador *) &a->pag, n->pagina);
    TR(EV_VISIT, .a = id_de(n));
    *nivel = profundidade;

    for (i = 0; i < n->n; i++) {
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        if (n->chaves[i] == chave) {
            TR(EV_MSG, .a = STR_ACHOU);
            return OK;
        }
    }

    TR(EV_UNVISIT, .a = id_de(n));
    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

/* ---- a varredura --------------------------------------------------------
 *
 * É a operação que a estrutura existe para fazer. Nenhum nó interno é tocado,
 * nenhuma página é lida duas vezes, e o custo total é o número de folhas.   */

int arvore_bmais_varrer(const ArvoreBMais *a)
{
    const NoBM *f = a->primeira;

    if (f == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_MSG, .a = STR_VARRENDO);

    while (f != NULL) {
        LER_PAGINA((Paginador *) &a->pag, f->pagina);
        TR(EV_VISIT, .a = id_de(f));
        TR(EV_UNVISIT, .a = id_de(f));
        f = f->proxima;
    }

    return OK;
}

/* ---- inserção -----------------------------------------------------------
 *
 * Divide na descida, como a árvore B, e pelo mesmo motivo de disco: o pai
 * sempre tem espaço para a chave que sobe, e a inserção é uma passada só.   */

/* Divide o filho `i` de `pai`, que está cheio.
 *
 * Aqui está a diferença de uma palavra que separa as duas estruturas. Se o nó
 * cheio é INTERNO, a chave do meio SOBE — sai de baixo e passa a existir só
 * em cima, exatamente como na árvore B. Se é FOLHA, ela é COPIADA: continua
 * embaixo, onde os dados moram, e o pai fica com um retrato dela para
 * rotear.
 *
 * E é na folha que o elo é remendado: a folha nova entra na corrente entre a
 * que foi dividida e a que vinha depois dela. Esquecer esta linha dá uma
 * árvore que passa em toda busca e perde metade das chaves na varredura. */
static int dividir_filho(ArvoreBMais *a, NoBM *pai, int i)
{
    NoBM  *cheio = pai->filhos[i];
    NoBM  *novo = no_novo(a, cheio->folha);
    int    t = a->t;
    elem_t sobe;
    int    j;

    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }

    TR(EV_VISIT, .a = id_de(cheio));
    TR(EV_MSG, .a = STR_PAGINA_CHEIA);
    TR(EV_MSG, .a = STR_DIVIDE);

    if (cheio->folha) {
        /* A folha cheia tem 2t-1 chaves: ficam t à esquerda e t-1 à direita,
         * e nenhuma se perde no caminho. */
        novo->n = t - 1;
        for (j = 0; j < t - 1; j++) {
            novo->chaves[j] = cheio->chaves[j + t];
        }
        cheio->n = t;

        novo->proxima = cheio->proxima;
        cheio->proxima = novo;

        sobe = novo->chaves[0];
        TR(EV_MSG, .a = STR_COPIA_CHAVE);
    } else {
        novo->n = t - 1;
        for (j = 0; j < t - 1; j++) {
            novo->chaves[j] = cheio->chaves[j + t];
        }
        for (j = 0; j < t; j++) {
            novo->filhos[j] = cheio->filhos[j + t];
        }
        cheio->n = t - 1;

        sobe = cheio->chaves[t - 1];
        TR(EV_MSG, .a = STR_SOBE_CHAVE);
    }

    /* Abre espaço no pai para o filho novo e para a chave que sobe. */
    for (j = pai->n; j > i; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    pai->filhos[i + 1] = novo;

    for (j = pai->n - 1; j >= i; j--) {
        pai->chaves[j + 1] = pai->chaves[j];
    }
    pai->chaves[i] = sobe;
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

static int inserir_nao_cheio(ArvoreBMais *a, NoBM *n, elem_t chave)
{
    LER_PAGINA(&a->pag, n->pagina);
    TR(EV_VISIT, .a = id_de(n));

    if (n->folha) {
        int i = n->n - 1;

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

    {
        int i = rota(n, chave);

        TR(EV_UNVISIT, .a = id_de(n));

        if (n->filhos[i]->n == 2 * a->t - 1) {
            int rc = dividir_filho(a, n, i);

            if (rc != OK) {
                return rc;
            }
            /* O separador novo está em chaves[i], e a regra da B+ manda o
             * igual para a direita. */
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            if (chave >= n->chaves[i]) {
                i++;
            }
        }

        return inserir_nao_cheio(a, n->filhos[i], chave);
    }
}

int arvore_bmais_inserir(ArvoreBMais *a, elem_t chave)
{
    int nivel = -1;

    if (arvore_bmais_buscar(a, chave, &nivel) == OK) {
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
        a->primeira = a->raiz;

        publicar(a->raiz);
        ESCREVER_PAGINA(&a->pag, a->raiz->pagina);
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(a->raiz));
        /* A corrente de folhas começa aqui, e o rótulo `início` é a ponta
         * dela na tela. */
        TR(EV_PTR_SET, .a = PTR_INICIO, .b = id_de(a->primeira));

        a->total++;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
        return OK;
    }

    if (a->raiz->n == 2 * a->t - 1) {
        NoBM *nova = no_novo(a, 0);
        int   rc;

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
 * A garantia é a da árvore B: nunca se desce para um nó com t-1 chaves. Antes
 * de entrar nele, ele ganha uma — emprestada de um irmão, ou fundindo-se com
 * um. Quando a chave é enfim removida, lá na folha, há folga de sobra.
 *
 * O que muda é o papel da chave do pai. Entre nós internos ela é DADO e a
 * rotação a move de verdade, como na árvore B. Entre folhas ela é SEPARADOR:
 * o empréstimo move a chave de uma folha para a outra e depois REESCREVE o
 * separador com a menor chave da folha da direita, e a fusão simplesmente
 * apaga o separador — ele não desce, porque não é dado de ninguém.          */

static int remover_de(ArvoreBMais *a, NoBM *n, elem_t chave);

/* Funde o filho i, o filho i+1 e — só entre nós internos — a chave i do pai. */
static void fundir(ArvoreBMais *a, NoBM *pai, int i)
{
    NoBM *esq = pai->filhos[i];
    NoBM *dir = pai->filhos[i + 1];
    int   base;
    int   j;

    TR(EV_MSG, .a = STR_FUNDE);
    TR(EV_VISIT, .a = id_de(esq));
    TR(EV_VISIT, .a = id_de(dir));

    if (esq->folha) {
        /* O separador do pai some, e ninguém perde chave: ele era um retrato
         * da menor chave da direita, e essa chave continua ali, na folha
         * fundida. */
        base = esq->n;
        for (j = 0; j < dir->n; j++) {
            esq->chaves[base + j] = dir->chaves[j];
        }
        esq->n += dir->n;

        /* O elo passa por cima da folha que morre. */
        esq->proxima = dir->proxima;
    } else {
        /* Aqui a chave do pai é dado, e desce para o meio: é exatamente a que
         * subiu quando este nó foi dividido. */
        TR(EV_MSG, .a = STR_DESCE_CHAVE);
        base = esq->n;
        esq->chaves[base] = pai->chaves[i];
        for (j = 0; j < dir->n; j++) {
            esq->chaves[base + 1 + j] = dir->chaves[j];
        }
        for (j = 0; j <= dir->n; j++) {
            esq->filhos[base + 1 + j] = dir->filhos[j];
        }
        esq->n += dir->n + 1;
    }

    for (j = i + 1; j < pai->n; j++) {
        pai->chaves[j - 1] = pai->chaves[j];
    }
    for (j = i + 2; j <= pai->n; j++) {
        pai->filhos[j - 1] = pai->filhos[j];
    }
    pai->n--;

    TR(EV_UNVISIT, .a = id_de(dir));
    no_liberar(dir);

    publicar(esq);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, esq->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);
    TR(EV_UNVISIT, .a = id_de(esq));
}

static void emprestar_do_anterior(ArvoreBMais *a, NoBM *pai, int i)
{
    NoBM *filho = pai->filhos[i];
    NoBM *irmao = pai->filhos[i - 1];
    int   j;

    TR(EV_MSG, .a = STR_EMPRESTA_ESQ);
    TR(EV_VISIT, .a = id_de(irmao));

    for (j = filho->n - 1; j >= 0; j--) {
        filho->chaves[j + 1] = filho->chaves[j];
    }

    if (filho->folha) {
        /* A maior chave do irmão atravessa para cá, e o separador passa a ser
         * ela: menor que tudo o que está aqui, maior que tudo o que ficou lá. */
        filho->chaves[0] = irmao->chaves[irmao->n - 1];
        pai->chaves[i - 1] = filho->chaves[0];
    } else {
        for (j = filho->n; j >= 0; j--) {
            filho->filhos[j + 1] = filho->filhos[j];
        }
        filho->filhos[0] = irmao->filhos[irmao->n];

        filho->chaves[0] = pai->chaves[i - 1];
        pai->chaves[i - 1] = irmao->chaves[irmao->n - 1];
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

static void emprestar_do_proximo(ArvoreBMais *a, NoBM *pai, int i)
{
    NoBM *filho = pai->filhos[i];
    NoBM *irmao = pai->filhos[i + 1];
    int   j;

    TR(EV_MSG, .a = STR_EMPRESTA_DIR);
    TR(EV_VISIT, .a = id_de(irmao));

    if (filho->folha) {
        /* A menor chave do irmão atravessa para cá — de verdade, porque é
         * dado. O separador é reescrito depois, quando já se sabe qual chave
         * sobrou como a menor da direita. */
        filho->chaves[filho->n] = irmao->chaves[0];
    } else {
        /* A rotação clássica: a chave do pai desce, a primeira do irmão sobe
         * para o lugar dela. */
        filho->chaves[filho->n] = pai->chaves[i];
        filho->filhos[filho->n + 1] = irmao->filhos[0];
        pai->chaves[i] = irmao->chaves[0];
    }

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

    if (filho->folha) {
        pai->chaves[i] = irmao->chaves[0];
    }

    publicar(filho);
    publicar(irmao);
    publicar(pai);
    ESCREVER_PAGINA(&a->pag, filho->pagina);
    ESCREVER_PAGINA(&a->pag, irmao->pagina);
    ESCREVER_PAGINA(&a->pag, pai->pagina);
    TR(EV_UNVISIT, .a = id_de(irmao));
}

/* Garante que o filho `i` tenha ao menos t chaves antes de se descer nele. */
static void preencher(ArvoreBMais *a, NoBM *pai, int i)
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

static int remover_de(ArvoreBMais *a, NoBM *n, elem_t chave)
{
    int i;

    LER_PAGINA(&a->pag, n->pagina);
    TR(EV_VISIT, .a = id_de(n));

    if (n->folha) {
        for (i = 0; i < n->n; i++) {
            TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
            if (n->chaves[i] == chave) {
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
        }

        TR(EV_UNVISIT, .a = id_de(n));
        return ERR_NAO_ENCONTRADO;
    }

    i = rota(n, chave);
    TR(EV_UNVISIT, .a = id_de(n));

    if (n->filhos[i]->n < a->t) {
        preencher(a, n, i);
        /* A rota é recalculada, e não corrigida na mão: `preencher` pode ter
         * fundido dois filhos e apagado um separador, e o caminho certo é o
         * que os separadores de AGORA dizem. Na árvore B isto vira um caso
         * especial ("era o último filho?"); aqui a pergunta é a mesma de
         * antes, feita de novo. */
        i = rota(n, chave);
    }

    return remover_de(a, n->filhos[i], chave);
}

int arvore_bmais_remover(ArvoreBMais *a, elem_t chave)
{
    int rc;

    if (a->raiz == NULL) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    rc = remover_de(a, a->raiz, chave);

    /* O colapso vem antes de olhar o rc, pela mesma razão que na árvore B: a
     * descida reestrutura mesmo quando a chave não existe, e uma fusão na
     * raiz pode tê-la deixado sem chave nenhuma. */
    if (a->raiz->n == 0) {
        NoBM *velha = a->raiz;

        if (velha->folha) {
            a->raiz = NULL;
            a->primeira = NULL;
            TR(EV_PTR_SET, .a = PTR_INICIO, .b = 0);
        } else {
            a->raiz = velha->filhos[0];
        }
        TR(EV_PTR_SET, .a = PTR_RAIZ, .b = id_de(a->raiz));
        no_liberar(velha);
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

static void liberar(NoBM *n)
{
    int i;

    if (n == NULL) {
        return;
    }
    if (!n->folha) {
        for (i = 0; i <= n->n; i++) {
            liberar(n->filhos[i]);
        }
    }
    no_liberar(n);
}

void arvore_bmais_limpar(ArvoreBMais *a)
{
    int quantos = a->total;

    liberar(a->raiz);
    a->raiz = NULL;
    a->primeira = NULL;
    a->total = 0;

    TR(EV_PTR_SET, .a = PTR_RAIZ, .b = 0);
    TR(EV_PTR_SET, .a = PTR_INICIO, .b = 0);
    if (quantos > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -quantos);
    }
}

void arvore_bmais_destruir(ArvoreBMais *a)
{
    if (a == NULL) {
        return;
    }
    trace_set_enabled(0);
    liberar(a->raiz);
    trace_set_enabled(1);
    free(a);
}

int arvore_bmais_tamanho(const ArvoreBMais *a)
{
    return a->total;
}

int arvore_bmais_grau(const ArvoreBMais *a)
{
    return a->t;
}

int arvore_bmais_altura(const ArvoreBMais *a)
{
    const NoBM *n = a->raiz;
    int         h = 0;

    while (n != NULL) {
        h++;
        if (n->folha) {
            break;
        }
        n = n->filhos[0];
    }
    return h;
}

int arvore_bmais_folhas(const ArvoreBMais *a)
{
    const NoBM *f = a->primeira;
    int         quantas = 0;

    while (f != NULL) {
        quantas++;
        f = f->proxima;
    }
    return quantas;
}

long arvore_bmais_leituras(const ArvoreBMais *a)
{
    return paginador_leituras(&a->pag);
}

long arvore_bmais_escritas(const ArvoreBMais *a)
{
    return paginador_escritas(&a->pag);
}

/* ---- invariantes ------------------------------------------------------- */

static int contem_em(const NoBM *n, elem_t chave)
{
    int i;

    if (n == NULL) {
        return 0;
    }
    if (n->folha) {
        for (i = 0; i < n->n; i++) {
            if (n->chaves[i] == chave) {
                return 1;
            }
        }
        return 0;
    }

    i = 0;
    while (i < n->n && chave >= n->chaves[i]) {
        i++;
    }
    return contem_em(n->filhos[i], chave);
}

int arvore_bmais_contem(const ArvoreBMais *a, elem_t chave)
{
    return contem_em(a->raiz, chave);
}

int arvore_bmais_em_ordem(const ArvoreBMais *a, elem_t *saida, int max)
{
    const NoBM *f = a->primeira;
    int         k = 0;
    int         i;

    while (f != NULL) {
        for (i = 0; i < f->n; i++) {
            if (k >= max) {
                return k;
            }
            saida[k++] = f->chaves[i];
        }
        f = f->proxima;
    }
    return k;
}

/* Confere forma, ocupação e faixa, e devolve a profundidade das folhas (ou -1
 * se algo estiver errado).
 *
 * A faixa é a da B+: `menor <= chave < maior`. O separador pertence à direita,
 * e essa é a única diferença em relação à conferência da árvore B — a mesma
 * que o roteamento faz. */
static int conferir(const NoBM *n, int t, int eh_raiz, elem_t menor, elem_t maior,
                    int tem_menor, int tem_maior, int *chaves_em_folha)
{
    int i;
    int profundidade = -1;

    if (n == NULL) {
        return -1;
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
        if (tem_menor && n->chaves[i] < menor) {
            return -1;
        }
        if (tem_maior && n->chaves[i] >= maior) {
            return -1;
        }
    }

    if (n->folha) {
        *chaves_em_folha += n->n;
        return 1;
    }

    for (i = 0; i <= n->n; i++) {
        int sub = conferir(n->filhos[i], t, 0,
                           (i > 0) ? n->chaves[i - 1] : menor,
                           (i < n->n) ? n->chaves[i] : maior,
                           (i > 0) ? 1 : tem_menor,
                           (i < n->n) ? 1 : tem_maior,
                           chaves_em_folha);

        if (sub < 0) {
            return -1;
        }
        if (profundidade < 0) {
            profundidade = sub;
        } else if (sub != profundidade) {
            return -1;
        }
    }

    return profundidade + 1;
}

/* A folha mais à esquerda, descendo. Tem que ser a mesma que a árvore guarda
 * como `primeira`: se não for, a varredura começa no meio. */
static const NoBM *mais_a_esquerda(const NoBM *n)
{
    while (n != NULL && !n->folha) {
        n = n->filhos[0];
    }
    return n;
}

int arvore_bmais_valida(const ArvoreBMais *a)
{
    const NoBM *f;
    int         em_folha = 0;
    int         pela_corrente = 0;
    int         anterior = 0;
    int         primeiro = 1;

    if (a->raiz == NULL) {
        return a->total == 0 && a->primeira == NULL;
    }

    if (conferir(a->raiz, a->t, 1, 0, 0, 0, 0, &em_folha) < 0) {
        return 0;
    }

    /* O dado mora só nas folhas: o tamanho é a soma delas, e as chaves dos
     * nós internos não entram na conta. */
    if (em_folha != a->total) {
        return 0;
    }

    if (a->primeira != mais_a_esquerda(a->raiz)) {
        return 0;
    }

    /* A corrente: crescente, sem repetir, e passando por todas as chaves que
     * a descida encontrou. */
    for (f = a->primeira; f != NULL; f = f->proxima) {
        int i;

        if (!f->folha) {
            return 0;
        }
        for (i = 0; i < f->n; i++) {
            if (!primeiro && f->chaves[i] <= anterior) {
                return 0;
            }
            anterior = f->chaves[i];
            primeiro = 0;
            pela_corrente++;
        }
    }

    return pela_corrente == em_folha;
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * A capacidade da sessão é o GRAU t, como na árvore B.                      */

static void *vt_criar(int capacidade)
{
    return arvore_bmais_criar(capacidade);
}

static void vt_destruir(void *s)
{
    arvore_bmais_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return arvore_bmais_inserir(s, valor);
}

static void vt_limpar(void *s)
{
    arvore_bmais_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return arvore_bmais_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return arvore_bmais_grau(s);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return arvore_bmais_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return arvore_bmais_remover(s, valor);
}

/* Só a varredura em ordem. Pré e pós-ordem existem numa árvore em que os nós
 * internos guardam dado; aqui eles guardam roteiro, e visitá-los numa ordem
 * ou noutra não diria nada sobre o conteúdo. */
static int vt_percurso(const void *s, int ordem)
{
    if (ordem != PERC_EM_ORDEM) {
        return ERR_ARG_INVALIDO;
    }
    return arvore_bmais_varrer(s);
}

const TAD_Linear ARVORE_B_MAIS = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar,
    .remover_valor = vt_remover_valor,
    .percurso = vt_percurso,
};
