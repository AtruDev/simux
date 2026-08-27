/* core/sort/medida.c — os dois contadores que descrevem um algoritmo.
 *
 * Comparações e escritas são o que separa a seleção da bolha: as duas fazem
 * O(n²) comparações, e só uma faz O(n) escritas. Sem contá-las, a aba de
 * ordenação seria um vetor bonito piscando.
 *
 * São globais porque a alternativa — um contexto passado a cada algoritmo —
 * poluiria a assinatura que o painel de código exibe, e não há concorrência:
 * o core roda numa thread só, dentro de uma chamada de ds_call. */

#include "ds/ordenacao.h"

static long g_comparacoes;
static long g_escritas;

void medida_zerar(void)
{
    g_comparacoes = 0;
    g_escritas = 0;
}

void medida_comparou(void)
{
    g_comparacoes++;
}

void medida_escreveu(int quantas)
{
    g_escritas += quantas;
}

long medida_comparacoes(void)
{
    return g_comparacoes;
}

long medida_escritas(void)
{
    return g_escritas;
}
