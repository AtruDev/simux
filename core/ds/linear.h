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

    /* ---- opcionais da árvore -------------------------------------------- *
     * A árvore entrou por aqui pelo mesmo motivo que a lista: um segundo
     * caminho de despacho dobraria api.c e a interface inteira, para uma
     * estrutura que já cabe em inserir/remover/consultar/buscar.
     *
     * O que ela tem de próprio são dois. Remover por VALOR — numa árvore a
     * posição não existe, e é essa remoção que traz os três casos que a aula
     * inteira gira em torno. E percorrer, que não é remoção nem consulta: é a
     * estrutura sendo lida numa ordem que ela mesma define.
     *
     * `remover` sem argumento continua valendo, e quer dizer "remova o
     * menor" — é o que a fila de prioridade faria, e é o que o vetor
     * ordenado já faz.                                                      */
    int   (*remover_valor)(void *s, elem_t valor);
    /* ordem é um PERC_* de ids.h */
    int   (*percurso)(const void *s, int ordem);
} TAD_Linear;

extern const TAD_Linear PILHA_ENC;
extern const TAD_Linear PILHA_VET;
extern const TAD_Linear FILA_ENC;
extern const TAD_Linear FILA_VET;
extern const TAD_Linear LISTA_SIMPLES;
extern const TAD_Linear LISTA_DUPLA;
extern const TAD_Linear LISTA_CIRCULAR;

/* As duas buscas sobre o MESMO vetor ordenado. Todos os ponteiros das duas
 * tabelas são iguais menos `buscar`, e esse um é a aula inteira: no modo
 * comparar, a mesma sequência roda nas duas e o contador de comparações mostra
 * O(n) contra O(log n). */
extern const TAD_Linear BUSCA_SEQ;
extern const TAD_Linear BUSCA_BIN;

extern const TAD_Linear ABB;
extern const TAD_Linear AVL;

/* As quatro tabelas hash. As três abertas são o mesmo código com uma sondagem
 * diferente, decidida em `criar` — e `remover` e `consultar` sem argumento
 * ficam nulos nas quatro: numa tabela hash não existe "o primeiro" nem "o
 * menor", porque a ordem dos elementos é acidente da função hash. */
extern const TAD_Linear HASH_ENC;
extern const TAD_Linear HASH_LINEAR;
extern const TAD_Linear HASH_QUAD;
extern const TAD_Linear HASH_DUPLO;

/* A capacidade da sessão dela é o GRAU t, e não um número de células: é o
 * único parâmetro que uma árvore B tem, e é ele que muda a forma inteira. */
extern const TAD_Linear ARVORE_B;

#endif /* DS_LINEAR_H */
