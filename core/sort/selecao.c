/* core/sort/selecao.c — ordenação por seleção do mínimo.
 *
 * Faz as mesmas O(n²) comparações da bolha e apenas O(n) escritas — uma troca
 * por passada, no máximo. É o par que justifica o painel de métricas ter duas
 * linhas em vez de uma: com os dois contadores lado a lado, "as duas são n²"
 * deixa de ser a história inteira.
 *
 * Em compensação, ela não tem saída antecipada: vetor já ordenado custa o
 * mesmo que vetor invertido. Rodar as duas sobre "quase ordenado" no modo
 * corrida é o jeito mais curto de mostrar isso. */

#define TR_SRC SRC_SELECAO

#include "ds/ordenacao.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

int selecao_ordenar(elem_t *v, int n)
{
    int i;

    for (i = 0; i < n - 1; i++) {
        int menor = i;
        int j;

        TR(EV_PHASE, .a = STR_PROCURANDO_MIN, .b = i);
        TR(EV_ARR_RANGE, .a = i, .b = n - 1);
        TR(EV_PTR_SET, .a = PTR_I, .b = i);
        TR(EV_PTR_SET, .a = PTR_MIN, .b = menor);

        for (j = i + 1; j < n; j++) {
            TR(EV_PTR_SET, .a = PTR_J, .b = j);
            COMPARAR(j, menor);
            if (v[j] < v[menor]) {
                menor = j;
                TR(EV_PTR_SET, .a = PTR_MIN, .b = menor);
            }
        }

        /* O `if` existe para o contador não mentir: trocar uma célula com ela
         * mesma é escrita nenhuma, e contá-la inflaria justamente a métrica
         * em que este algoritmo é bom. */
        if (menor != i) {
            TROCAR(v, i, menor);
        }
        TR(EV_ARR_MARK, .a = i, .b = TAG_ORDENADO);
    }

    if (n > 0) {
        TR(EV_ARR_MARK, .a = n - 1, .b = TAG_ORDENADO);
    }

    return OK;
}
