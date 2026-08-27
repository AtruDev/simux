/* core/hash/hash_enc.c — tabela hash com encadeamento separado.
 *
 * Cada balde é uma lista. Colidiu, entra na lista. A tabela nunca enche, e o
 * preço disso é um malloc por elemento e uma cadeia que cresce — com uma
 * função hash ruim, ou com m pequeno demais, a tabela vira uma lista ligada
 * com um vetor na frente, e a busca volta a ser O(n).
 *
 * O desenho usa os dois mundos do vocabulário ao mesmo tempo, e sem inventar
 * evento: o arranjo de baldes é EV_ARR_INIT mais um EV_ARR_WRITE por balde, e
 * o valor escrito na célula é o ID DO NÓ que está na cabeça da cadeia. É
 * literalmente o que a estrutura guarda — um vetor de ponteiros —, e é o que
 * deixa o frontend seguir a cadeia a partir da célula.
 *
 * A inserção entra pela CABEÇA, e não pelo fim. Não é preguiça: é O(1), é o
 * que a matéria faz, e é o que mantém a promessa da tabela hash. Inserir no
 * fim exigiria andar a cadeia, e aí o custo de inserir passaria a depender do
 * tamanho dela. */

#define TR_SRC SRC_HASH_ENC

#include "ds/hash.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/trace.h"

#include "linear.h"

enum { SLOT_PROX = 0 };

typedef struct No {
    elem_t     valor;
    struct No *prox;
} No;

struct HashEnc {
    No **baldes;
    int  m;
    int  n;
};

/* h(k) = k mod m.
 *
 * O `% ` do C devolve negativo para chave negativa, e um índice negativo é
 * acesso fora do vetor. A correção é uma linha, e é o bug que quase todo
 * mundo escreve na primeira vez. */
static int balde_de(const HashEnc *h, elem_t chave)
{
    int i = (int) (chave % h->m);

    return (i < 0) ? i + h->m : i;
}

HashEnc *hash_enc_criar(int m)
{
    HashEnc *h;
    int      i;

    if (m <= 0) {
        return NULL;
    }

    h = malloc(sizeof *h);
    if (h == NULL) {
        return NULL;
    }

    h->baldes = malloc((size_t) m * sizeof *h->baldes);
    if (h->baldes == NULL) {
        free(h);
        return NULL;
    }

    h->m = m;
    h->n = 0;

    TR(EV_ARR_INIT, .a = m);
    for (i = 0; i < m; i++) {
        h->baldes[i] = NULL;
        /* Célula com 0 é balde vazio: 0 é o id que o idmap dá para NULL. */
        TR(EV_ARR_WRITE, .a = i, .b = 0);
    }

    return h;
}

int hash_enc_inserir(HashEnc *h, elem_t valor)
{
    int  i = balde_de(h, valor);
    No  *atual;
    No  *novo;

    TR(EV_PTR_SET, .a = PTR_BALDE, .b = i);
    TR(EV_MSG, .a = STR_BALDE);

    /* Andar a cadeia antes de inserir é o que impede repetido — e é também
     * onde o custo da colisão aparece: cada nó visitado é uma comparação. */
    for (atual = h->baldes[i]; atual != NULL; atual = atual->prox) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);
        if (atual->valor == valor) {
            TR(EV_MSG, .a = STR_JA_EXISTE);
            TR(EV_UNVISIT, .a = id_de(atual));
            return OK;
        }
        TR(EV_UNVISIT, .a = id_de(atual));
    }

    if (h->baldes[i] != NULL) {
        /* Dois valores diferentes no mesmo balde: é a definição de colisão, e
         * é o número que mede se o m escolhido presta. */
        TR(EV_MSG, .a = STR_COLISAO);
        TR(EV_COUNT, .a = CNT_COLISOES, .b = +1);
    }

    novo = malloc(sizeof *novo);
    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }
    novo->valor = valor;
    novo->prox = h->baldes[i];

    TR(EV_NODE_NEW, .a = id_de(novo), .b = valor);
    TR(EV_EDGE_SET, .a = id_de(novo), .b = SLOT_PROX, .c = id_de(novo->prox));
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    h->baldes[i] = novo;
    TR(EV_ARR_WRITE, .a = i, .b = id_de(novo));

    h->n++;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    return OK;
}

int hash_enc_buscar(const HashEnc *h, elem_t valor, int *balde)
{
    int i = balde_de(h, valor);
    No *atual;

    TR(EV_PTR_SET, .a = PTR_BALDE, .b = i);
    TR(EV_MSG, .a = STR_BALDE);

    for (atual = h->baldes[i]; atual != NULL; atual = atual->prox) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (atual->valor == valor) {
            TR(EV_MSG, .a = STR_ACHOU);
            *balde = i;
            return OK;
        }
        TR(EV_UNVISIT, .a = id_de(atual));
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

int hash_enc_remover(HashEnc *h, elem_t valor)
{
    int  i = balde_de(h, valor);
    No  *anterior = NULL;
    No  *atual;

    TR(EV_PTR_SET, .a = PTR_BALDE, .b = i);
    TR(EV_MSG, .a = STR_BALDE);

    for (atual = h->baldes[i]; atual != NULL; atual = atual->prox) {
        TR(EV_VISIT, .a = id_de(atual));
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (atual->valor == valor) {
            /* Duas religações possíveis, e a diferença é visível na tela: sair
             * da cabeça muda a CÉLULA do balde, sair do meio muda a ARESTA do
             * nó anterior. */
            if (anterior == NULL) {
                h->baldes[i] = atual->prox;
                TR(EV_ARR_WRITE, .a = i, .b = id_de(atual->prox));
            } else {
                anterior->prox = atual->prox;
                TR(EV_EDGE_SET, .a = id_de(anterior), .b = SLOT_PROX,
                   .c = id_de(atual->prox));
            }

            TR(EV_UNVISIT, .a = id_de(atual));
            TR(EV_NODE_FREE, .a = id_de(atual));
            id_esquece(atual);
            free(atual);
            TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);

            h->n--;
            TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
            return OK;
        }

        TR(EV_UNVISIT, .a = id_de(atual));
        anterior = atual;
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

void hash_enc_limpar(HashEnc *h)
{
    int i;

    for (i = 0; i < h->m; i++) {
        No *atual = h->baldes[i];

        while (atual != NULL) {
            No *seguinte = atual->prox;

            TR(EV_NODE_FREE, .a = id_de(atual));
            id_esquece(atual);
            free(atual);
            TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
            atual = seguinte;
        }

        if (h->baldes[i] != NULL) {
            h->baldes[i] = NULL;
            TR(EV_ARR_WRITE, .a = i, .b = 0);
        }
    }

    if (h->n > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -h->n);
        h->n = 0;
    }
}

int hash_enc_tamanho(const HashEnc *h)
{
    return h->n;
}

int hash_enc_baldes(const HashEnc *h)
{
    return h->m;
}

int hash_enc_maior_cadeia(const HashEnc *h)
{
    int maior = 0;
    int i;

    for (i = 0; i < h->m; i++) {
        const No *atual;
        int       quantos = 0;

        for (atual = h->baldes[i]; atual != NULL; atual = atual->prox) {
            quantos++;
        }
        if (quantos > maior) {
            maior = quantos;
        }
    }
    return maior;
}

int hash_enc_contem(const HashEnc *h, elem_t valor)
{
    const No *atual;

    for (atual = h->baldes[balde_de(h, valor)]; atual != NULL;
         atual = atual->prox) {
        if (atual->valor == valor) {
            return 1;
        }
    }
    return 0;
}

void hash_enc_destruir(HashEnc *h)
{
    if (h == NULL) {
        return;
    }
    trace_set_enabled(0);
    hash_enc_limpar(h);
    trace_set_enabled(1);
    free(h->baldes);
    free(h);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * `remover` e `consultar` sem argumento ficam nulos: numa tabela hash não
 * existe "o primeiro" nem "o menor" — a ordem dos elementos é acidente da
 * função hash, e devolver um deles seria inventar uma semântica. api.c
 * responde ERR_OP_DESCONHECIDA a quem pedir, e a interface esconde os botões.
 */

static void *vt_criar(int capacidade)
{
    return hash_enc_criar(capacidade);
}

static void vt_destruir(void *s)
{
    hash_enc_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return hash_enc_inserir(s, valor);
}

static void vt_limpar(void *s)
{
    hash_enc_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return hash_enc_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return hash_enc_baldes(s);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return hash_enc_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return hash_enc_remover(s, valor);
}

const TAD_Linear HASH_ENC = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar,
    .remover_valor = vt_remover_valor,
};
