/* core/include/ds/fila.h — o TAD Fila.
 *
 * Mesma ideia da pilha: uma interface, duas implementações, sufixo para os
 * símbolos não colidirem, despacho pelo vtable TAD_Linear.
 *
 * A versão com vetor é a circular. Não é um detalhe de implementação: é o
 * assunto. Sem dar a volta, uma fila em vetor gastaria o espaço da frente a
 * cada remoção e ficaria inutilizável depois de cap inserções. */

#ifndef DS_FILA_H
#define DS_FILA_H

#include "ds/tipos.h"

/* ---- encadeada: sem limite, um malloc por elemento ---------------------- */

typedef struct FilaEnc FilaEnc;

FilaEnc *fila_enc_criar(void);
void     fila_enc_destruir(FilaEnc *f);

int      fila_enc_enfileirar(FilaEnc *f, elem_t valor);
int      fila_enc_desenfileirar(FilaEnc *f, elem_t *saida);
int      fila_enc_frente(const FilaEnc *f, elem_t *saida);
void     fila_enc_limpar(FilaEnc *f);
int      fila_enc_tamanho(const FilaEnc *f);

/* ---- com vetor: circular, de capacidade fixa ---------------------------- */

typedef struct FilaVet FilaVet;

FilaVet *fila_vet_criar(int capacidade);
void     fila_vet_destruir(FilaVet *f);

int      fila_vet_enfileirar(FilaVet *f, elem_t valor);
int      fila_vet_desenfileirar(FilaVet *f, elem_t *saida);
int      fila_vet_frente(const FilaVet *f, elem_t *saida);
void     fila_vet_limpar(FilaVet *f);
int      fila_vet_tamanho(const FilaVet *f);
int      fila_vet_capacidade(const FilaVet *f);

#endif /* DS_FILA_H */
