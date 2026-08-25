/* core/ds/linear.h — a tabela que despacha entre implementações do mesmo TAD.
 *
 * É a definição de TAD que a matéria dá, virando código: a interface é uma, as
 * implementações são várias, e quem chama não sabe qual está do outro lado.
 * Trocar pilha encadeada por pilha com vetor em tempo de execução vira uma
 * atribuição de ponteiro.
 *
 * Pilha e fila cabem na mesma tabela porque as operações são as mesmas em
 * forma: inserir, remover, consultar. O que muda é de que ponta — e é
 * justamente isso que a animação mostra.
 *
 * Sem campo de nome aqui de propósito. Texto voltado ao usuário vive no i18n
 * do frontend, e uma string dentro do núcleo seria o primeiro passo para
 * alguém devolvê-la pela fronteira. */

#ifndef DS_LINEAR_H
#define DS_LINEAR_H

#include "ds/tipos.h"

typedef struct {
    /* capacidade é ignorada pelas implementações encadeadas */
    void *(*criar)(int capacidade);
    void  (*destruir)(void *s);

    int   (*inserir)(void *s, elem_t valor);
    int   (*remover)(void *s, elem_t *saida);
    int   (*consultar)(const void *s, elem_t *saida);
    void  (*limpar)(void *s);

    int   (*tamanho)(const void *s);
    /* -1 quando não há limite, que é o caso das encadeadas */
    int   (*capacidade)(const void *s);
} TAD_Linear;

extern const TAD_Linear PILHA_ENC;
extern const TAD_Linear PILHA_VET;
extern const TAD_Linear FILA_ENC;
extern const TAD_Linear FILA_VET;

#endif /* DS_LINEAR_H */
