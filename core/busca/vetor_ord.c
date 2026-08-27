/* core/busca/vetor_ord.c — vetor ordenado, busca sequencial e busca binária.
 *
 * Um arquivo só para as duas buscas, e isso é deliberado. No modo comparar, os
 * dois painéis de código exibem ESTE arquivo, cada um numa função e numa linha
 * diferente: as duas rodando a mesma busca, uma andando o vetor inteiro e a
 * outra cortando pela metade, com o mesmo dado à vista. Separar em dois .c
 * daria dois painéis que parecem não ter relação.
 *
 * O que muda entre as duas implementações é uma função. Tudo o mais — o
 * armazenamento, a inserção ordenada, a remoção — é compartilhado, porque é
 * assim que a comparação fica honesta: se a sequencial guardasse na ordem de
 * chegada, o contador estaria medindo duas diferenças ao mesmo tempo.
 *
 * A inserção ordenada é O(n) e aparece na tela como tal: cada elemento
 * deslocado é uma escrita. É o preço da busca binária, pago na hora de
 * inserir, e é a troca que a estrutura de dados faz. */

#define TR_SRC SRC_VETOR_ORD

#include "ds/busca.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "linear.h"

struct VetorOrd {
    elem_t *dados;
    int     cap;
    int     n;
};

VetorOrd *vetor_ord_criar(int capacidade)
{
    VetorOrd *v;

    if (capacidade <= 0) {
        return NULL;
    }

    v = malloc(sizeof *v);
    if (v == NULL) {
        return NULL;
    }

    v->dados = malloc((size_t) capacidade * sizeof *v->dados);
    if (v->dados == NULL) {
        free(v);
        return NULL;
    }

    v->cap = capacidade;
    v->n = 0;

    TR(EV_ARR_INIT, .a = capacidade);
    TR(EV_ARR_RANGE, .a = 0, .b = -1);

    return v;
}

/* Insere mantendo a ordem: acha o lugar, abre espaço, escreve.
 *
 * Achar o lugar é uma busca sequencial de trás para a frente, e ela é contada
 * como comparação — o custo de inserir num vetor ordenado é real e aparece no
 * painel. */
int vetor_ord_inserir(VetorOrd *v, elem_t valor)
{
    int i;

    if (v->n == v->cap) {
        TR(EV_MSG, .a = STR_VETOR_CHEIO);
        return ERR_CHEIA;
    }

    /* O valor a inserir fica "em mãos" enquanto o resto desliza — é o mesmo
     * auxiliar de uma célula da ordenação por inserção, e é ele que dá sentido
     * ao `.c = 1` das comparações abaixo. */
    TR(EV_AUX_INIT, .a = 1);
    TR(EV_AUX_WRITE, .a = 0, .b = valor);
    if (v->n > 0) {
        TR(EV_MSG, .a = STR_DESLOCANDO);
    }

    /* Anda de trás para a frente empurrando quem for maior. O laço termina
     * sozinho no lugar certo — não é preciso buscar antes e deslocar depois. */
    for (i = v->n - 1; i >= 0; i--) {
        TR(EV_ARR_COMPARE, .a = i, .b = 0, .c = 1);
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (v->dados[i] <= valor) {
            break;
        }

        v->dados[i + 1] = v->dados[i];
        TR(EV_ARR_WRITE, .a = i + 1, .b = v->dados[i + 1]);
        TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);
    }

    v->dados[i + 1] = valor;
    TR(EV_ARR_WRITE, .a = i + 1, .b = valor);
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);

    v->n++;
    TR(EV_ARR_RANGE, .a = 0, .b = v->n - 1);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);

    return OK;
}

/* ---- busca sequencial ---------------------------------------------------
 *
 * Olha uma célula de cada vez, da esquerda para a direita. Custo O(n), e ela
 * não usa em nada o fato de o vetor estar ordenado — trocar o vetor por um
 * desordenado daria exatamente o mesmo código.
 *
 * A única esperteza é parar quando passar do valor procurado: num vetor
 * ordenado, o que vem depois só cresce. Corta o caso "não está lá" pela
 * metade, na média, e continua sendo O(n).                                */

int vetor_ord_buscar_seq(const VetorOrd *v, elem_t valor, int *pos)
{
    int i;

    /* A chave procurada, à vista: toda comparação daqui é contra ela. */
    TR(EV_AUX_INIT, .a = 1);
    TR(EV_AUX_WRITE, .a = 0, .b = valor);

    for (i = 0; i < v->n; i++) {
        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = i);
        TR(EV_ARR_COMPARE, .a = i, .b = 0, .c = 1);
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (v->dados[i] == valor) {
            TR(EV_ARR_MARK, .a = i, .b = TAG_PIVO);
            TR(EV_MSG, .a = STR_ACHOU);
            *pos = i;
            return OK;
        }
        if (v->dados[i] > valor) {
            break;      /* ordenado: daqui para a frente só cresce */
        }
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

/* ---- busca binária ------------------------------------------------------
 *
 * Olha o meio e joga fora metade do que sobrou. Custo O(log n) — com n = 1000,
 * são 10 comparações contra 1000.
 *
 * O EV_ARR_RANGE a cada passo é o desenho da aula: a faixa viva encolhe pela
 * metade, e o que saiu dela apaga na tela. É por isso que a faixa entrou no
 * vocabulário de eventos lá na Fase 0 — ela existe para este momento.       */

int vetor_ord_buscar_bin(const VetorOrd *v, elem_t valor, int *pos)
{
    int lo = 0;
    int hi = v->n - 1;

    TR(EV_AUX_INIT, .a = 1);
    TR(EV_AUX_WRITE, .a = 0, .b = valor);

    while (lo <= hi) {
        /* lo + (hi - lo) / 2, e não (lo + hi) / 2: a segunda estoura o int
         * para vetor grande, e é o bug clássico da busca binária — ficou
         * quase uma década dentro da biblioteca padrão do Java. */
        int meio = lo + (hi - lo) / 2;

        TR(EV_ARR_RANGE, .a = lo, .b = hi);
        TR(EV_PTR_SET, .a = PTR_I, .b = lo);
        TR(EV_PTR_SET, .a = PTR_J, .b = hi);
        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = meio);
        TR(EV_ARR_COMPARE, .a = meio, .b = 0, .c = 1);
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (v->dados[meio] == valor) {
            TR(EV_ARR_MARK, .a = meio, .b = TAG_PIVO);
            TR(EV_MSG, .a = STR_ACHOU);
            *pos = meio;
            return OK;
        }

        if (v->dados[meio] < valor) {
            /* O procurado é maior: tudo à esquerda do meio, e o meio, saem. */
            TR(EV_MSG, .a = STR_DESCARTA_ESQ);
            lo = meio + 1;
        } else {
            TR(EV_MSG, .a = STR_DESCARTA_DIR);
            hi = meio - 1;
        }
    }

    /* A faixa vazia é o que significa "não está lá": lo passou hi. */
    TR(EV_ARR_RANGE, .a = 0, .b = v->n - 1);
    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

/* ---- o resto do TAD ---------------------------------------------------- */

int vetor_ord_remover(VetorOrd *v, elem_t *saida)
{
    int i;

    if (v->n == 0) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = 0);
    *saida = v->dados[0];

    /* Remover o menor desloca todo o resto uma casa para a esquerda. É o mesmo
     * O(n) da inserção, pelo mesmo motivo: contíguo tem preço. */
    for (i = 1; i < v->n; i++) {
        v->dados[i - 1] = v->dados[i];
        TR(EV_ARR_WRITE, .a = i - 1, .b = v->dados[i - 1]);
        TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);
    }

    v->n--;
    TR(EV_ARR_MARK, .a = v->n, .b = TAG_LIVRE);
    TR(EV_ARR_RANGE, .a = 0, .b = v->n - 1);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);

    return OK;
}

int vetor_ord_menor(const VetorOrd *v, elem_t *saida)
{
    if (v->n == 0) {
        TR(EV_MSG, .a = STR_LISTA_VAZIA);
        return ERR_VAZIA;
    }

    TR(EV_ARR_READ, .a = 0);
    *saida = v->dados[0];
    return OK;
}

void vetor_ord_limpar(VetorOrd *v)
{
    while (v->n > 0) {
        v->n--;
        TR(EV_ARR_MARK, .a = v->n, .b = TAG_LIVRE);
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    }
    TR(EV_ARR_RANGE, .a = 0, .b = -1);
}

int vetor_ord_tamanho(const VetorOrd *v)
{
    return v->n;
}

int vetor_ord_capacidade(const VetorOrd *v)
{
    return v->cap;
}

int vetor_ord_ordenado(const VetorOrd *v)
{
    int i;

    for (i = 1; i < v->n; i++) {
        if (v->dados[i - 1] > v->dados[i]) {
            return 0;
        }
    }
    return 1;
}

void vetor_ord_destruir(VetorOrd *v)
{
    if (v == NULL) {
        return;
    }
    free(v->dados);
    free(v);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * Duas tabelas sobre a mesma estrutura. Todos os ponteiros são iguais menos
 * um, e esse um é a aula inteira.                                          */

static void *vt_criar(int capacidade)
{
    return vetor_ord_criar(capacidade);
}

static void vt_destruir(void *s)
{
    vetor_ord_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return vetor_ord_inserir(s, valor);
}

static int vt_remover(void *s, elem_t *saida)
{
    return vetor_ord_remover(s, saida);
}

static int vt_consultar(const void *s, elem_t *saida)
{
    return vetor_ord_menor(s, saida);
}

static void vt_limpar(void *s)
{
    vetor_ord_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return vetor_ord_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return vetor_ord_capacidade(s);
}

static int vt_buscar_seq(const void *s, elem_t valor, int *pos)
{
    return vetor_ord_buscar_seq(s, valor, pos);
}

static int vt_buscar_bin(const void *s, elem_t valor, int *pos)
{
    return vetor_ord_buscar_bin(s, valor, pos);
}

/* inserir_em e remover_em ficam nulos: num vetor ordenado a posição é
 * consequência do valor, não escolha de quem chama. Pedir "insira na posição
 * 3" aqui é operação desconhecida, e api.c responde isso. */

const TAD_Linear BUSCA_SEQ = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .remover = vt_remover,
    .consultar = vt_consultar,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar_seq,
};

const TAD_Linear BUSCA_BIN = {
    .criar = vt_criar,
    .destruir = vt_destruir,
    .inserir = vt_inserir,
    .remover = vt_remover,
    .consultar = vt_consultar,
    .limpar = vt_limpar,
    .tamanho = vt_tamanho,
    .capacidade = vt_capacidade,
    .buscar = vt_buscar_bin,
};
