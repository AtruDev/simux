/* core/sort/ordenacao.c — a tabela que liga ALG_* à função.
 *
 * O equivalente, do lado da ordenação, ao vtable TAD_Linear das estruturas: a
 * interface é uma, as implementações são seis, e quem chama não sabe qual
 * está do outro lado. Acrescentar o heapsort é uma linha aqui, uma no enum de
 * ids.h e um arquivo em core/sort/ — a interface do frontend se monta a
 * partir do enum, e não precisa saber. */

#include "ds/ordenacao.h"

#include <stddef.h>

#include "ds/ids.h"

/* Na ordem de ds_alg. O array ser indexado pelo enum, e não percorrido, é o
 * que faz um algoritmo fora de ordem virar erro na hora de ler — e é por isso
 * que a checagem de tamanho abaixo existe. */
static const OrdenaFn TABELA[] = {
    bolha_ordenar,
    selecao_ordenar,
    insercao_ordenar,
    shell_ordenar,
    quick_ordenar,
    merge_ordenar,
    externa_ordenar,
};

/* Se alguém acrescentar um ALG_ e esquecer a linha na tabela, o build para
 * aqui em vez de a aba mostrar o algoritmo errado. */
typedef char tabela_completa[
    (sizeof TABELA / sizeof TABELA[0] == ALG_COUNT) ? 1 : -1];

OrdenaFn ordenacao_de(int alg)
{
    if (alg < 0 || alg >= ALG_COUNT) {
        return NULL;
    }
    return TABELA[alg];
}
