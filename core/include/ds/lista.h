/* core/include/ds/lista.h — o TAD Lista, nas três implementações.
 *
 * As três guardam os mesmos elementos na mesma ordem e respondem igual a toda
 * sequência de operações — é isso que o fuzz diferencial verifica. O que muda
 * é o CUSTO, e é o custo que a animação mostra:
 *
 *   simples    um ponteiro por nó. Inserir no fim anda a lista inteira.
 *   dupla      prox e ant, mais um ponteiro para o fim. Inserir no fim é O(1),
 *              e a travessia começa pela ponta mais perto da posição pedida.
 *   circular   só um ponteiro, para o ÚLTIMO nó — cujo prox é o primeiro.
 *              Início e fim ficam os dois a um passo de distância.
 *
 * A posição é 0-based. Inserir aceita de 0 a n (n insere no fim); remover e
 * buscar aceitam de 0 a n-1.
 *
 * As três operações sem posição — inserir, remover, consultar — agem todas no
 * INÍCIO. É a ponta onde as três são O(1), e escolher uma só ponta para as
 * três é o que mantém a tabela TAD_Linear coerente. */

#ifndef DS_LISTA_H
#define DS_LISTA_H

#include "ds/tipos.h"

/* ---- simplesmente encadeada -------------------------------------------- */

typedef struct ListaSimples ListaSimples;

ListaSimples *lista_simples_criar(void);
void          lista_simples_destruir(ListaSimples *l);

int           lista_simples_inserir(ListaSimples *l, int pos, elem_t valor);
int           lista_simples_remover(ListaSimples *l, int pos, elem_t *saida);
int           lista_simples_buscar(const ListaSimples *l, elem_t valor,
                                   int *pos);
int           lista_simples_primeiro(const ListaSimples *l, elem_t *saida);
void          lista_simples_limpar(ListaSimples *l);
int           lista_simples_tamanho(const ListaSimples *l);

/* ---- duplamente encadeada ---------------------------------------------- */

typedef struct ListaDupla ListaDupla;

ListaDupla   *lista_dupla_criar(void);
void          lista_dupla_destruir(ListaDupla *l);

int           lista_dupla_inserir(ListaDupla *l, int pos, elem_t valor);
int           lista_dupla_remover(ListaDupla *l, int pos, elem_t *saida);
int           lista_dupla_buscar(const ListaDupla *l, elem_t valor, int *pos);
int           lista_dupla_primeiro(const ListaDupla *l, elem_t *saida);
void          lista_dupla_limpar(ListaDupla *l);
int           lista_dupla_tamanho(const ListaDupla *l);

/* ---- circular ----------------------------------------------------------- */

typedef struct ListaCircular ListaCircular;

ListaCircular *lista_circular_criar(void);
void           lista_circular_destruir(ListaCircular *l);

int            lista_circular_inserir(ListaCircular *l, int pos, elem_t valor);
int            lista_circular_remover(ListaCircular *l, int pos, elem_t *saida);
int            lista_circular_buscar(const ListaCircular *l, elem_t valor,
                                     int *pos);
int            lista_circular_primeiro(const ListaCircular *l, elem_t *saida);
void           lista_circular_limpar(ListaCircular *l);
int            lista_circular_tamanho(const ListaCircular *l);

#endif /* DS_LISTA_H */
