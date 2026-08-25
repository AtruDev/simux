/* core/ds/fila_vet.c — fila circular sobre vetor.
 *
 * Duas coisas aqui são exatamente onde a aula trava, e as duas viram imagem:
 *
 * O wrap-around. Quando o fim passa do último índice, ele volta para o zero, e
 * a fila fica com a ordem lógica separada da ordem física: com inicio = 5 e
 * fim = 2 num vetor de 8, ela *parece* invertida na tela.
 *
 * Cheia contra vazia. Sem o contador n, `fim + 1 == inicio` significaria as
 * duas coisas ao mesmo tempo. É por isso que n existe, e é por isso que ele
 * fica sempre visível na interface. */

#define TR_SRC SRC_FILA_VET

#include "ds/fila.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "linear.h"

struct FilaVet {
    elem_t *dados;
    int     cap;
    int     inicio;     /* índice do primeiro                              */
    int     fim;        /* índice do último                                */
    int     n;          /* o que distingue cheia de vazia                  */
};

FilaVet *fila_vet_criar(int capacidade)
{
    FilaVet *f;

    if (capacidade <= 0) {
        return NULL;
    }

    f = malloc(sizeof *f);
    if (f == NULL) {
        return NULL;
    }

    f->dados = malloc((size_t) capacidade * sizeof *f->dados);
    if (f->dados == NULL) {
        free(f);
        return NULL;
    }

    f->cap = capacidade;
    f->inicio = 0;
    f->fim = -1;
    f->n = 0;

    TR(EV_ARR_INIT, .a = capacidade);
    TR(EV_PTR_SET, .a = PTR_FRENTE, .b = -1);
    TR(EV_PTR_SET, .a = PTR_FIM, .b = -1);

    return f;
}

int fila_vet_enfileirar(FilaVet *f, elem_t valor)
{
    int novo_fim;

    if (f->n == f->cap) {
        TR(EV_MSG, .a = STR_FILA_CHEIA);
        return ERR_CHEIA;
    }

    novo_fim = (f->fim + 1) % f->cap;

    /* O momento didático: o índice deu a volta em vez de crescer. */
    if (f->n > 0 && novo_fim < f->fim) {
        TR(EV_MSG, .a = STR_DEU_VOLTA);
    }

    f->fim = novo_fim;
    f->dados[f->fim] = valor;
    TR(EV_ARR_WRITE, .a = f->fim, .b = valor);

    if (f->n == 0) {
        f->inicio = f->fim;
        TR(EV_PTR_SET, .a = PTR_FRENTE, .b = f->inicio);
    }

    f->n++;
    TR(EV_PTR_SET, .a = PTR_FIM, .b = f->fim);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);

    return OK;
}

int fila_vet_desenfileirar(FilaVet *f, elem_t *saida)
{
    if (f->n == 0) {
        TR(EV_MSG, .a = STR_FILA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = f->inicio);
    *saida = f->dados[f->inicio];

    /* A célula não é apagada: ela deixa de pertencer à fila, e continua
     * reservada. Ver isso é metade do argumento contra a versão encadeada. */
    TR(EV_ARR_MARK, .a = f->inicio, .b = TAG_LIVRE);

    f->n--;

    if (f->n == 0) {
        /* Vazia de novo: recomeça do zero para a próxima inserção ficar
         * contígua, o que deixa a tela mais legível sem mudar a lógica. */
        f->inicio = 0;
        f->fim = -1;
        TR(EV_PTR_SET, .a = PTR_FRENTE, .b = -1);
        TR(EV_PTR_SET, .a = PTR_FIM, .b = -1);
    } else {
        int novo_inicio = (f->inicio + 1) % f->cap;

        if (novo_inicio < f->inicio) {
            TR(EV_MSG, .a = STR_DEU_VOLTA);
        }
        f->inicio = novo_inicio;
        TR(EV_PTR_SET, .a = PTR_FRENTE, .b = f->inicio);
    }

    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);

    return OK;
}

int fila_vet_frente(const FilaVet *f, elem_t *saida)
{
    if (f->n == 0) {
        TR(EV_MSG, .a = STR_FILA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = f->inicio);
    *saida = f->dados[f->inicio];

    return OK;
}

void fila_vet_limpar(FilaVet *f)
{
    while (f->n > 0) {
        TR(EV_ARR_MARK, .a = f->inicio, .b = TAG_LIVRE);
        f->inicio = (f->inicio + 1) % f->cap;
        f->n--;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }

    f->inicio = 0;
    f->fim = -1;
    TR(EV_PTR_SET, .a = PTR_FRENTE, .b = -1);
    TR(EV_PTR_SET, .a = PTR_FIM, .b = -1);
}

int fila_vet_tamanho(const FilaVet *f)
{
    return f->n;
}

int fila_vet_capacidade(const FilaVet *f)
{
    return f->cap;
}

void fila_vet_destruir(FilaVet *f)
{
    if (f == NULL) {
        return;
    }
    free(f->dados);
    free(f);
}

/* ---- adaptação para o vtable ------------------------------------------- */

static void *vt_criar(int capacidade)
{
    return fila_vet_criar(capacidade);
}

static void vt_destruir(void *s)
{
    fila_vet_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return fila_vet_enfileirar(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return fila_vet_desenfileirar(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return fila_vet_frente(s, saida);
}

static void vt_limpar(void *s)
{
    fila_vet_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return fila_vet_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return fila_vet_capacidade(s);
}

const TAD_Linear FILA_VET = {
    vt_criar, vt_destruir,
    vt_inserir, vt_remover, vt_consultar, vt_limpar,
    vt_tamanho, vt_capacidade,
};
