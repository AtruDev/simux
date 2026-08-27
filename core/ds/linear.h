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
 * A lista não cabe nessa forma: nela a posição é argumento, não consequência.
 * Em vez de um segundo caminho de despacho — que dobraria api.c e a interface
 * inteira —, ela entra pelos três membros OPCIONAIS do fim da tabela, nulos
 * para pilha e fila. Nulo aqui quer dizer "este TAD não tem posição", e é o
 * que api.c testa para devolver ERR_OP_DESCONHECIDA.
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

    /* ---- opcionais: nulos em quem não tem posição ---------------------- *
     * A posição é 0-based. Inserir aceita de 0 a n (n = no fim); remover e
     * buscar aceitam de 0 a n-1. Fora disso é ERR_ARG_INVALIDO.            */
    int   (*inserir_em)(void *s, int pos, elem_t valor);
    int   (*remover_em)(void *s, int pos, elem_t *saida);
    /* devolve OK e a posição por ponteiro, ou ERR_NAO_ENCONTRADO */
    int   (*buscar)(const void *s, elem_t valor, int *pos);
} TAD_Linear;

extern const TAD_Linear PILHA_ENC;
extern const TAD_Linear PILHA_VET;
extern const TAD_Linear FILA_ENC;
extern const TAD_Linear FILA_VET;
extern const TAD_Linear LISTA_SIMPLES;
extern const TAD_Linear LISTA_DUPLA;
extern const TAD_Linear LISTA_CIRCULAR;

#endif /* DS_LINEAR_H */
