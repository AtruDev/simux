/* core/sort/passos.h — os três passos que carregam métrica.
 *
 * São macros, e não funções, por dois motivos que se somam. O primeiro é o
 * `__LINE__` de dentro de TR: expandindo no ponto de chamada, a linha que o
 * painel destaca é a do laço do algoritmo, e não a de uma função auxiliar
 * comum a todos eles. O segundo é `TR_SRC`, que cada .c define para si — a
 * mesma macro emite SRC_BOLHA dentro de bolha.c e SRC_QUICK dentro de quick.c.
 *
 * Só existem três porque só três precisam encostar num contador. Marcar uma
 * célula, mover um índice ou anunciar uma fase é um TR direto, e continua
 * legível como TR.
 *
 * COMPARAR não faz a comparação: ela anuncia que a próxima linha vai comparar.
 * O `if (v[j] > v[j + 1])` fica visível no algoritmo, que é justamente o que
 * o aluno veio ler — esconder o operador dentro de uma macro pouparia uma
 * linha e custaria a aula.
 *
 * Cada uma emite DOIS eventos: o que aconteceu e o incremento do contador. É
 * a mesma convenção de pilha_vet.c, e não é redundância — o painel de
 * métricas acumula os deltas do EV_COUNT ao reproduzir, e deduzir o contador
 * dos eventos de vetor do lado do JS daria dois lugares para a mesma verdade.
 * O contador em C, esse, existe para o modo empírico, que roda sem trace. */

#ifndef DS_SORT_PASSOS_H
#define DS_SORT_PASSOS_H

#include "ds/ids.h"
#include "ds/ordenacao.h"
#include "ds/tipos.h"
#include "ds/trace.h"

/* Compara duas células do vetor. */
#define COMPARAR(i, j) do {                                     \
    medida_comparou();                                          \
    TR(EV_ARR_COMPARE, .a = (int) (i), .b = (int) (j));         \
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);                \
} while (0)

/* Compara a célula i com o valor em mãos — o auxiliar de uma posição só que
 * a inserção e o shell mantêm fora do vetor. É o `.c = 1` documentado em
 * ids.h; o mundo do auxiliar já existe, e não precisou de evento novo. */
#define COMPARAR_MAO(i) do {                                    \
    medida_comparou();                                          \
    TR(EV_ARR_COMPARE, .a = (int) (i), .b = 0, .c = 1);         \
    TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);                \
} while (0)

#define TROCAR(vet, i, j) do {                                  \
    int    de_ = (int) (i);                                     \
    int    para_ = (int) (j);                                   \
    elem_t guarda_ = (vet)[de_];                                \
    (vet)[de_] = (vet)[para_];                                  \
    (vet)[para_] = guarda_;                                     \
    medida_escreveu(2);                                         \
    TR(EV_ARR_SWAP, .a = de_, .b = para_);                      \
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +2);                   \
} while (0)

#define ESCREVER(vet, i, val) do {                              \
    int    onde_ = (int) (i);                                   \
    elem_t quanto_ = (val);                                     \
    (vet)[onde_] = quanto_;                                     \
    medida_escreveu(1);                                         \
    TR(EV_ARR_WRITE, .a = onde_, .b = (int) quanto_);           \
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);                   \
} while (0)

/* Escrita no buffer auxiliar do merge. Conta como escrita: o auxiliar é
 * memória de verdade, e é ela que faz o mergesort custar O(n) de espaço —
 * o número que o compara com o quicksort. */
#define ESCREVER_AUX(vet, i, val) do {                          \
    int    onde_ = (int) (i);                                   \
    elem_t quanto_ = (val);                                     \
    (vet)[onde_] = quanto_;                                     \
    medida_escreveu(1);                                         \
    TR(EV_AUX_WRITE, .a = onde_, .b = (int) quanto_);           \
    TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);                   \
} while (0)

/* O valor em mãos, desenhado como um auxiliar de uma célula. A inserção e o
 * shell tiram um elemento do vetor para abrir espaço à direita; sem mostrar
 * onde ele foi parar, o deslocamento parece apagar dados. */
#define NA_MAO(val) do {                                        \
    TR(EV_AUX_WRITE, .a = 0, .b = (int) (val));                 \
} while (0)

#endif /* DS_SORT_PASSOS_H */
