/* core/include/ds/pilha.h — o TAD Pilha.
 *
 * A interface é uma só; as implementações são duas e ganham sufixo, porque
 * compilam no mesmo binário e não podem definir os mesmos símbolos. Por
 * enquanto existe só a encadeada — a com vetor entra na Fase 2, junto do
 * vtable que despacha entre as duas. */

#ifndef DS_PILHA_H
#define DS_PILHA_H

#include "ds/tipos.h"

typedef struct PilhaEnc PilhaEnc;

PilhaEnc *pilha_enc_criar(void);
void      pilha_enc_destruir(PilhaEnc *p);

int       pilha_enc_push(PilhaEnc *p, elem_t valor);
int       pilha_enc_pop(PilhaEnc *p, elem_t *saida);
int       pilha_enc_topo(const PilhaEnc *p, elem_t *saida);
void      pilha_enc_limpar(PilhaEnc *p);
int       pilha_enc_tamanho(const PilhaEnc *p);

#endif /* DS_PILHA_H */
