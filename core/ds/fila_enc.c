/* core/ds/fila_enc.c — fila com alocação dinâmica.
 *
 * Dois ponteiros nomeados em vez de um: entra pelo fim, sai pela frente. É a
 * diferença inteira para a pilha, e a animação mostra exatamente isso. */

#define TR_SRC SRC_FILA_ENC

#include "ds/fila.h"

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

struct FilaEnc {
    No *frente;
    No *fim;
    int n;
};

FilaEnc *fila_enc_criar(void)
{
    FilaEnc *f = malloc(sizeof *f);

    if (f != NULL) {
        f->frente = NULL;
        f->fim = NULL;
        f->n = 0;
    }
    return f;
}

int fila_enc_enfileirar(FilaEnc *f, elem_t valor)
{
    No *novo = malloc(sizeof *novo);

    if (novo == NULL) {
        return ERR_SEM_MEMORIA;
    }

    novo->valor = valor;
    novo->prox = NULL;
    TR(EV_NODE_NEW, .a = id_de(novo), .b = valor);
    TR(EV_EDGE_SET, .a = id_de(novo), .b = 0, .c = 0);

    if (f->fim == NULL) {
        /* Fila vazia: o nó novo é ao mesmo tempo a frente e o fim. */
        f->frente = novo;
        TR(EV_PTR_SET, .a = PTR_FRENTE, .b = id_de(novo));
    } else {
        f->fim->prox = novo;
        TR(EV_EDGE_SET, .a = id_de(f->fim), .b = 0, .c = id_de(novo));
    }

    f->fim = novo;
    f->n++;
    TR(EV_PTR_SET, .a = PTR_FIM, .b = id_de(novo));
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    return OK;
}

int fila_enc_desenfileirar(FilaEnc *f, elem_t *saida)
{
    No *morto;

    if (f->frente == NULL) {
        TR(EV_MSG, .a = STR_FILA_VAZIA);
        return ERR_VAZIA;
    }

    morto = f->frente;
    TR(EV_VISIT, .a = id_de(morto));
    *saida = morto->valor;

    f->frente = morto->prox;
    TR(EV_PTR_SET, .a = PTR_FRENTE, .b = id_de(f->frente));

    if (f->frente == NULL) {
        /* Saiu o último: o fim também precisa soltar, senão fica apontando
         * para memória liberada — é o vazamento clássico desta estrutura. */
        f->fim = NULL;
        TR(EV_PTR_SET, .a = PTR_FIM, .b = 0);
    }

    TR(EV_NODE_FREE, .a = id_de(morto));
    id_esquece(morto);
    free(morto);

    f->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);

    return OK;
}

int fila_enc_frente(const FilaEnc *f, elem_t *saida)
{
    if (f->frente == NULL) {
        TR(EV_MSG, .a = STR_FILA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_VISIT, .a = id_de(f->frente));
    *saida = f->frente->valor;
    TR(EV_UNVISIT, .a = id_de(f->frente));

    return OK;
}

void fila_enc_limpar(FilaEnc *f)
{
    while (f->frente != NULL) {
        No *morto = f->frente;

        f->frente = morto->prox;
        TR(EV_PTR_SET, .a = PTR_FRENTE, .b = id_de(f->frente));
        TR(EV_NODE_FREE, .a = id_de(morto));
        id_esquece(morto);
        free(morto);

        f->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }

    f->fim = NULL;
    TR(EV_PTR_SET, .a = PTR_FIM, .b = 0);
}

int fila_enc_tamanho(const FilaEnc *f)
{
    return f->n;
}

void fila_enc_destruir(FilaEnc *f)
{
    if (f == NULL) {
        return;
    }
    fila_enc_limpar(f);
    free(f);
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    (void) capacidade;
    return fila_enc_criar();
}

static void vt_destruir(void *s)
{
    fila_enc_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return fila_enc_enfileirar(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return fila_enc_desenfileirar(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return fila_enc_frente(s, saida);
}

static void vt_limpar(void *s)
{
    fila_enc_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return fila_enc_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    (void) s;
    return -1;
}

const TAD_Linear FILA_ENC = {
    vt_criar, vt_destruir,
    vt_inserir, vt_remover, vt_consultar, vt_limpar,
    vt_tamanho, vt_capacidade,
};
