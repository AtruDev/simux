/* core/include/ds/tipos.h — o tipo dos dados guardados nas estruturas.
 *
 * Decisão fechada: as estruturas guardam int. Isso mantém ds_call() como
 * quatro inteiros e o trace como um Int32Array, e dispensa ponteiro de
 * função para comparar. O typedef existe para que uma eventual
 * generalização futura seja uma troca em um lugar só. */

#ifndef DS_TIPOS_H
#define DS_TIPOS_H

typedef int elem_t;

#endif /* DS_TIPOS_H */
