/* core/sort/shell.c — inserção com gap, o primeiro salto de desempenho.
 *
 * É a mesma inserção do arquivo ao lado, com uma diferença de uma palavra: em
 * vez de comparar com o vizinho, compara com quem está `gap` casas atrás.
 * Isso deixa um elemento distante do seu lugar viajar em poucos passos, em
 * vez de uma casa por troca — que é exatamente o que trava a inserção pura.
 *
 * A sequência de gaps é a original de Shell, n/2, n/4, ..., 1. Não é a melhor
 * conhecida (Ciura e Sedgewick são bem melhores), e está aqui por ser a que a
 * matéria apresenta e a mais fácil de ver na tela: cada passada é uma
 * sub-sequência colorida, e a última passada é uma inserção comum sobre um
 * vetor já quase ordenado.
 *
 * O `gap` viaja no EV_PHASE, e é dele que o frontend tira a cor de cada
 * sub-sequência: as células com o mesmo `i % gap` são o mesmo grupo. */

#define TR_SRC SRC_SHELL

#include "ds/ordenacao.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "passos.h"

int shell_ordenar(elem_t *v, int n)
{
    int gap;

    if (n <= 0) {
        return OK;
    }

    TR(EV_AUX_INIT, .a = 1);

    for (gap = n / 2; gap > 0; gap /= 2) {
        int i;

        TR(EV_PHASE, .a = STR_GAP, .b = gap);
        TR(EV_ARR_RANGE, .a = 0, .b = n - 1);

        for (i = gap; i < n; i++) {
            elem_t chave = v[i];
            int    j;

            TR(EV_PTR_SET, .a = PTR_I, .b = i);
            TR(EV_ARR_READ, .a = i);
            NA_MAO(chave);

            for (j = i; j >= gap; j -= gap) {
                COMPARAR_MAO(j - gap);
                if (v[j - gap] <= chave) {
                    break;
                }
                ESCREVER(v, j, v[j - gap]);
                TR(EV_PTR_SET, .a = PTR_J, .b = j - gap);
            }

            ESCREVER(v, j, chave);
        }
    }

    return OK;
}
