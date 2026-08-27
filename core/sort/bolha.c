/* core/sort/bolha.c — ordenação por flutuação, com flag de parada.
 *
 * O algoritmo mais lento da lista e o mais fácil de ler, o que faz dele o
 * primeiro a ser instrumentado: se o vocabulário de eventos não desse conta
 * dele, não daria conta de nenhum.
 *
 * A flag `trocou` não é otimização de enfeite. É ela que separa a bolha da
 * bolha ingênua: com o vetor já ordenado, a primeira passada não troca nada e
 * o algoritmo sai em O(n) — e é isso que a distribuição "quase ordenado" da
 * aba mostra acontecendo. */

#define TR_SRC SRC_BOLHA

#include "ds/ordenacao.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

int bolha_ordenar(elem_t *v, int n)
{
    int i;

    for (i = 0; i < n - 1; i++) {
        int trocou = 0;
        int j;

        /* A cauda de tamanho i já está ordenada: cada passada empurra o maior
         * do trecho até o fim dele, e o trecho encolhe de um. */
        TR(EV_PHASE, .a = STR_PASSADA, .b = i + 1);
        TR(EV_ARR_RANGE, .a = 0, .b = n - 1 - i);

        for (j = 0; j < n - 1 - i; j++) {
            TR(EV_PTR_SET, .a = PTR_J, .b = j);
            COMPARAR(j, j + 1);
            if (v[j] > v[j + 1]) {
                TROCAR(v, j, j + 1);
                trocou = 1;
            }
        }

        TR(EV_ARR_MARK, .a = n - 1 - i, .b = TAG_ORDENADO);

        if (!trocou) {
            TR(EV_MSG, .a = STR_SEM_TROCAS);
            break;
        }
    }

    return OK;
}
