/* core/sort/cena.c — o vetor inicial, e por que ele é gerado aqui.
 *
 * Poderia ser JavaScript de dez linhas. Não é, por um motivo único e
 * suficiente: `Math.random` não tem semente. Gerado no C, com o xorshift de
 * ds/aleatorio.h, a mesma semente dá o mesmo vetor em qualquer máquina — que
 * é o que faz um link compartilhado abrir exatamente a mesma cena, e uma
 * falha do fuzz ser reexecutável.
 *
 * As distribuições não são enfeite. Complexidade média e pior caso serem
 * coisas diferentes é conteúdo de prova, e cada uma delas existe para tornar
 * uma dessas diferenças visível:
 *
 *   QUASE_ORDENADO       a inserção ganha do quicksort
 *   INVERSO              o pior caso da inserção, e o melhor da bolha para
 *                        contar trocas
 *   POUCOS_DISTINTOS     mata o quicksort de partição de Lomuto com pivô no
 *                        fim: quase tudo cai de um lado só
 *   ORDENADO             a bolha sai em O(n) por causa da flag; a seleção
 *                        continua custando o mesmo
 */

#define TR_SRC SRC_CENA

#include "ds/ordenacao.h"

#include <stddef.h>

#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

/* Quantas trocas embaralham um vetor "quase ordenado": uma a cada dez
 * posições, no mínimo uma. Poucas o bastante para a inserção brilhar, muitas
 * o bastante para não parecer já ordenado. */
static int quase_trocas(int n)
{
    int t = n / 10;

    return (t < 1) ? 1 : t;
}

/* Fisher-Yates. Embaralhar uma permutação de 1..n, em vez de sortear valores
 * soltos, garante que não haja repetido — o que deixa a barra de cada valor
 * ter altura própria e a ordenação ser visível sem ler número. */
static void embaralhar(elem_t *v, int n, Aleatorio *a)
{
    int i;

    for (i = n - 1; i > 0; i--) {
        int j = aleatorio_entre(a, 0, i);
        elem_t guarda = v[i];

        v[i] = v[j];
        v[j] = guarda;
    }
}

int cena_gerar(elem_t *v, int n, int dist, unsigned int semente,
               const elem_t *manual)
{
    Aleatorio a;
    int       i;

    if (v == NULL || n <= 0 || dist < 0 || dist >= DIST_COUNT) {
        return ERR_ARG_INVALIDO;
    }
    if (dist == DIST_MANUAL && manual == NULL) {
        return ERR_ARG_INVALIDO;
    }

    aleatorio_semear(&a, semente);

    for (i = 0; i < n; i++) {
        v[i] = i + 1;
    }

    switch (dist) {
    case DIST_ALEATORIO:
        embaralhar(v, n, &a);
        break;

    case DIST_QUASE_ORDENADO: {
        int t;

        for (t = 0; t < quase_trocas(n); t++) {
            int i1 = aleatorio_entre(&a, 0, n - 1);
            int i2 = aleatorio_entre(&a, 0, n - 1);
            elem_t guarda = v[i1];

            v[i1] = v[i2];
            v[i2] = guarda;
        }
        break;
    }

    case DIST_INVERSO:
        for (i = 0; i < n; i++) {
            v[i] = n - i;
        }
        break;

    case DIST_POUCOS_DISTINTOS: {
        /* Uns poucos valores repetidos muitas vezes. A raiz de n mantém a
         * proporção interessante em qualquer tamanho: com n = 100 dá 10
         * valores, com n = 10 dá 3. */
        int distintos = 2;

        while ((distintos + 1) * (distintos + 1) <= n) {
            distintos++;
        }
        for (i = 0; i < n; i++) {
            v[i] = aleatorio_entre(&a, 1, distintos) * (n / distintos);
        }
        break;
    }

    case DIST_ORDENADO:
        break;

    case DIST_MANUAL:
        for (i = 0; i < n; i++) {
            v[i] = manual[i];
        }
        break;

    default:
        return ERR_ARG_INVALIDO;
    }

    /* A tela precisa das células antes dos valores, e dos valores antes de
     * qualquer algoritmo rodar. Estas escritas não passam pelo contador: elas
     * são o estado inicial, não trabalho de ordenar. */
    TR(EV_ARR_INIT, .a = n);
    for (i = 0; i < n; i++) {
        TR(EV_ARR_WRITE, .a = i, .b = v[i]);
    }
    TR(EV_ARR_RANGE, .a = 0, .b = n - 1);
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = n);

    return OK;
}

int cena_ordenado(const elem_t *v, int n)
{
    int i;

    for (i = 1; i < n; i++) {
        if (v[i - 1] > v[i]) {
            return 0;
        }
    }
    return 1;
}

int cena_permutacao(const elem_t *antes, const elem_t *depois, int n)
{
    int i;
    int j;

    /* O(n²) e sem alocar nada. Os testes chamam isto com n pequeno, e um
     * multiconjunto de verdade aqui custaria um malloc no core para nada. */
    for (i = 0; i < n; i++) {
        int quantos_antes = 0;
        int quantos_depois = 0;

        for (j = 0; j < n; j++) {
            if (antes[j] == antes[i]) quantos_antes++;
            if (depois[j] == antes[i]) quantos_depois++;
        }
        if (quantos_antes != quantos_depois) {
            return 0;
        }
    }
    return 1;
}
