/* core/util/aleatorio.c — xorshift32.
 *
 * Doze linhas, período de 2^32-1, e suficiente para embaralhar vetor de
 * demonstração e sortear operação de fuzz. Não serve para criptografia, e não
 * é para isso que está aqui. */

#include "ds/aleatorio.h"

void aleatorio_semear(Aleatorio *a, uint32_t semente)
{
    /* Zero é ponto fixo do xorshift: a sequência inteira sairia zerada. */
    a->estado = (semente != 0u) ? semente : 0x9e3779b9u;
}

uint32_t aleatorio_proximo(Aleatorio *a)
{
    uint32_t x = a->estado;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a->estado = x;
    return x;
}

int aleatorio_entre(Aleatorio *a, int minimo, int maximo)
{
    uint32_t faixa;

    if (maximo <= minimo) {
        return minimo;
    }
    faixa = (uint32_t) (maximo - minimo) + 1u;
    return minimo + (int) (aleatorio_proximo(a) % faixa);
}
