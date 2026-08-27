/* core/sort/quick.c — quicksort com partição de Lomuto.
 *
 * Lomuto e não Hoare de propósito: o índice `i` marca a fronteira entre "já
 * sei que é menor ou igual ao pivô" e "ainda não olhei", e essa fronteira
 * anda uma casa de cada vez, da esquerda para a direita. Dá para acompanhar
 * com o dedo. A partição de Hoare é mais rápida e move dois índices em
 * sentidos opostos, o que na tela vira duas coisas acontecendo ao mesmo
 * tempo.
 *
 * O pivô é o último elemento, que é a escolha ingênua — e é a escolha certa
 * aqui: com ela, vetor ordenado e vetor de poucos valores distintos caem no
 * pior caso, O(n²), que é o que a distribuição "poucos valores distintos" da
 * aba existe para mostrar. Um pivô mediana-de-três esconderia justamente a
 * lição.
 *
 * A recursão, essa sim, é domada: o lado menor vai para a chamada recursiva e
 * o maior vira iteração. A profundidade fica O(log n) mesmo no pior caso, e o
 * tempo continua O(n²) — o modo empírico mede n = 25 600 sem estourar a pilha
 * do wasm, e a curva quadrática continua aparecendo. */

#define TR_SRC SRC_QUICK

#include "ds/ordenacao.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

/* Deixa o pivô na posição final e devolve onde ele parou. Tudo à esquerda
 * dela é menor ou igual; tudo à direita é maior. */
static int particionar(elem_t *v, int lo, int hi)
{
    elem_t pivo = v[hi];
    int    i = lo - 1;
    int    j;

    TR(EV_PHASE, .a = STR_PARTICIONANDO, .b = lo, .c = hi);
    TR(EV_ARR_MARK, .a = hi, .b = TAG_PIVO);

    for (j = lo; j < hi; j++) {
        TR(EV_PTR_SET, .a = PTR_J, .b = j);
        COMPARAR(j, hi);
        if (v[j] <= pivo) {
            i++;
            TR(EV_PTR_SET, .a = PTR_I, .b = i);
            if (i != j) {
                TROCAR(v, i, j);
            }
        }
    }

    TROCAR(v, i + 1, hi);
    TR(EV_ARR_MARK, .a = hi, .b = TAG_NENHUMA);
    TR(EV_ARR_MARK, .a = i + 1, .b = TAG_ORDENADO);

    return i + 1;
}

static void ordenar_faixa(elem_t *v, int lo, int hi)
{
    while (lo < hi) {
        int p;

        TR(EV_ARR_RANGE, .a = lo, .b = hi);
        p = particionar(v, lo, hi);

        /* Recursão no lado menor, iteração no maior: é o que limita a
         * profundidade a O(log n) sem mudar o algoritmo. */
        if (p - lo < hi - p) {
            ordenar_faixa(v, lo, p - 1);
            lo = p + 1;
        } else {
            ordenar_faixa(v, p + 1, hi);
            hi = p - 1;
        }
    }

    /* Faixa de um elemento só: ele já está no lugar. */
    if (lo == hi) {
        TR(EV_ARR_MARK, .a = lo, .b = TAG_ORDENADO);
    }
}

int quick_ordenar(elem_t *v, int n)
{
    if (n > 1) {
        ordenar_faixa(v, 0, n - 1);
    }
    return OK;
}
