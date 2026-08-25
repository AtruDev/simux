/* core/include/ds/pilha.h — o TAD Pilha.
 *
 * A interface é uma só; as implementações são duas e ganham sufixo, porque
 * compilam no mesmo binário e não podem definir os mesmos símbolos. O
 * despacho entre elas fica no vtable TAD_Linear, em core/ds/linear.h. */

#ifndef DS_PILHA_H
#define DS_PILHA_H

#include "ds/tipos.h"

/* ---- encadeada: sem limite, um malloc por elemento ---------------------- */

typedef struct PilhaEnc PilhaEnc;

PilhaEnc *pilha_enc_criar(void);
void      pilha_enc_destruir(PilhaEnc *p);

int       pilha_enc_push(PilhaEnc *p, elem_t valor);
int       pilha_enc_pop(PilhaEnc *p, elem_t *saida);
int       pilha_enc_topo(const PilhaEnc *p, elem_t *saida);
void      pilha_enc_limpar(PilhaEnc *p);
int       pilha_enc_tamanho(const PilhaEnc *p);

/* ---- com vetor: capacidade fixa, contígua, com overflow ----------------- */

typedef struct PilhaVet PilhaVet;

PilhaVet *pilha_vet_criar(int capacidade);
void      pilha_vet_destruir(PilhaVet *p);

int       pilha_vet_push(PilhaVet *p, elem_t valor);
int       pilha_vet_pop(PilhaVet *p, elem_t *saida);
int       pilha_vet_topo(const PilhaVet *p, elem_t *saida);
void      pilha_vet_limpar(PilhaVet *p);
int       pilha_vet_tamanho(const PilhaVet *p);
int       pilha_vet_capacidade(const PilhaVet *p);

#endif /* DS_PILHA_H */
