/* core/sort/merge.c — mergesort com buffer auxiliar único.
 *
 * O único da lista com pior caso O(n log n) garantido, e o único que paga
 * O(n) de memória por isso. É o par que faz a métrica de escritas valer a
 * pena: o quicksort ordena no lugar e o mergesort escreve tudo duas vezes por
 * nível — para o auxiliar, e de volta.
 *
 * O auxiliar é alocado uma vez, em merge_ordenar, e desce pela recursão. A
 * versão que aloca dentro de intercalar() é mais curta e faz O(n log n)
 * mallocs; além de lenta, ela faria o contador de alocações contar o
 * mergesort errado.
 *
 * A intercalação compara sempre duas células do VETOR e escreve no auxiliar,
 * e não o contrário. As duas ordens funcionam; esta é a que deixa toda
 * comparação ser entre dois índices que existem na tela. */

#define TR_SRC SRC_MERGE

#include "ds/ordenacao.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

/* Junta v[lo..meio] e v[meio+1..hi], ambos já ordenados. */
static void intercalar(elem_t *v, elem_t *aux, int lo, int meio, int hi)
{
    int i = lo;
    int j = meio + 1;
    int k = lo;

    TR(EV_PHASE, .a = STR_INTERCALANDO, .b = lo, .c = hi);
    TR(EV_ARR_RANGE, .a = lo, .b = hi);

    while (i <= meio && j <= hi) {
        TR(EV_PTR_SET, .a = PTR_I, .b = i);
        TR(EV_PTR_SET, .a = PTR_J, .b = j);
        COMPARAR(i, j);

        /* O <= é o que torna o mergesort estável: com valores iguais, o da
         * metade da esquerda sai primeiro. Trocar por < custaria a
         * estabilidade sem ganhar nada. */
        if (v[i] <= v[j]) {
            ESCREVER_AUX(aux, k, v[i]);
            i++;
        } else {
            ESCREVER_AUX(aux, k, v[j]);
            j++;
        }
        k++;
    }

    /* Uma das metades acabou; o resto da outra já está em ordem. */
    while (i <= meio) {
        ESCREVER_AUX(aux, k, v[i]);
        i++;
        k++;
    }
    while (j <= hi) {
        ESCREVER_AUX(aux, k, v[j]);
        j++;
        k++;
    }

    for (k = lo; k <= hi; k++) {
        ESCREVER(v, k, aux[k]);
    }
}

static void ordenar_faixa(elem_t *v, elem_t *aux, int lo, int hi)
{
    int meio;

    if (lo >= hi) {
        return;
    }

    /* lo + (hi - lo) / 2, e não (lo + hi) / 2: a segunda estoura o int para
     * vetor grande, e é o bug clássico da busca binária. */
    meio = lo + (hi - lo) / 2;

    TR(EV_PHASE, .a = STR_DIVIDINDO, .b = lo, .c = hi);
    TR(EV_ARR_RANGE, .a = lo, .b = hi);

    ordenar_faixa(v, aux, lo, meio);
    ordenar_faixa(v, aux, meio + 1, hi);
    intercalar(v, aux, lo, meio, hi);
}

int merge_ordenar(elem_t *v, int n)
{
    elem_t *aux;

    if (n <= 1) {
        return OK;
    }

    aux = malloc((size_t) n * sizeof *aux);
    if (aux == NULL) {
        return ERR_SEM_MEMORIA;
    }

    TR(EV_AUX_INIT, .a = n);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    ordenar_faixa(v, aux, 0, n - 1);

    free(aux);
    return OK;
}
