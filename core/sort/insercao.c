/* core/sort/insercao.c — inserção com deslocamento, e o valor em mãos.
 *
 * É o algoritmo que ganha do quicksort em vetor pequeno e em vetor quase
 * ordenado, e a aba de ordenação existe em boa parte para mostrar isso: no
 * modo corrida, sobre "quase ordenado", ele termina antes.
 *
 * O detalhe que a tela precisa contar é o `chave`. Ele sai do vetor, e as
 * células à direita são deslocadas por cima da posição que ele ocupava — sem
 * mostrar onde o valor foi parar, o deslocamento parece apagar dados. Daí ele
 * ser desenhado como um auxiliar de uma célula só, que é o que ele é. */

#define TR_SRC SRC_INSERCAO

#include "ds/ordenacao.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

int insercao_ordenar(elem_t *v, int n)
{
    int i;

    if (n <= 0) {
        return OK;
    }

    /* Uma célula só de auxiliar: o valor em mãos. */
    TR(EV_AUX_INIT, .a = 1);
    TR(EV_ARR_MARK, .a = 0, .b = TAG_ORDENADO);

    for (i = 1; i < n; i++) {
        elem_t chave = v[i];
        int    j = i - 1;

        TR(EV_PHASE, .a = STR_DESLOCANDO, .b = i);
        TR(EV_ARR_RANGE, .a = 0, .b = i);
        TR(EV_PTR_SET, .a = PTR_I, .b = i);
        TR(EV_ARR_READ, .a = i);
        NA_MAO(chave);

        /* Anda para a esquerda enquanto o vizinho for maior, empurrando cada
         * um uma casa para a direita. O laço para na primeira célula que já é
         * menor ou igual — é por isso que vetor quase ordenado sai barato. */
        while (j >= 0) {
            COMPARAR_MAO(j);
            if (v[j] <= chave) {
                break;
            }
            ESCREVER(v, j + 1, v[j]);
            TR(EV_PTR_SET, .a = PTR_J, .b = j);
            j--;
        }

        ESCREVER(v, j + 1, chave);
        TR(EV_ARR_MARK, .a = i, .b = TAG_ORDENADO);
    }

    return OK;
}
