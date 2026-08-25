/* core/include/ds/aleatorio.h — gerador pseudoaleatório próprio, com semente.
 *
 * Não é rand(). A implementação de rand() varia entre bibliotecas, então a
 * mesma semente daria cenas diferentes em máquinas diferentes — e cena
 * reproduzível é requisito: é o que faz um link compartilhado abrir o mesmo
 * exemplo, e o que faz uma falha de fuzz ser reexecutável. */

#ifndef DS_ALEATORIO_H
#define DS_ALEATORIO_H

#include <stdint.h>

typedef struct {
    uint32_t estado;
} Aleatorio;

void     aleatorio_semear(Aleatorio *a, uint32_t semente);
uint32_t aleatorio_proximo(Aleatorio *a);

/* Inteiro em [minimo, maximo], com os dois extremos incluídos. */
int      aleatorio_entre(Aleatorio *a, int minimo, int maximo);

#endif /* DS_ALEATORIO_H */
