/* core/include/ds/arvore.h — árvore binária de busca.
 *
 * A invariante é uma frase: para todo nó, tudo à esquerda é menor e tudo à
 * direita é maior. Dela sai o resto — a busca que desce por um caminho só, o
 * percurso em ordem que sai crescente, e a remoção com três casos.
 *
 * A remoção é o motivo de esta estrutura existir no simulador. Os dois
 * primeiros casos são triviais e o terceiro é onde todo aluno trava: o nó tem
 * dois filhos, e quem toma o lugar dele é o SUCESSOR EM ORDEM — o menor da
 * subárvore direita. A instrumentação mostra essa busca descendo, e é ela que
 * transforma a regra decorada em coisa vista.
 *
 * Repetidos não entram: numa ABB sem critério de desempate, inserir o mesmo
 * valor duas vezes torna a remoção ambígua. `inserir` devolve OK e não faz
 * nada, com a mensagem — recusar em silêncio esconderia a decisão.
 *
 * A altura não é devolvida por função: o frontend a calcula do desenho que já
 * tem, e assim o número no painel não tem como discordar da árvore na tela. */

#ifndef DS_ARVORE_H
#define DS_ARVORE_H

#include "ds/tipos.h"

typedef struct Abb Abb;

Abb *abb_criar(void);
void abb_destruir(Abb *a);

int  abb_inserir(Abb *a, elem_t valor);

/* Remove por valor, com os três casos. ERR_NAO_ENCONTRADO se não estiver lá. */
int  abb_remover(Abb *a, elem_t valor);

/* Remove o menor — o nó mais à esquerda. É o que `remover` sem argumento
 * significa no vtable, e é o que uma fila de prioridade faria. */
int  abb_remover_menor(Abb *a, elem_t *saida);
int  abb_menor(const Abb *a, elem_t *saida);

/* Devolve OK e a PROFUNDIDADE em que achou, ou ERR_NAO_ENCONTRADO.
 *
 * Profundidade e não posição: numa árvore, "onde está" é a que distância da
 * raiz — e é justamente o número que a comparação com a lista quer mostrar. */
int  abb_buscar(const Abb *a, elem_t valor, int *profundidade);

/* Percorre emitindo EV_VISIT na ordem pedida (PERC_* de ids.h). */
int  abb_percurso(const Abb *a, int ordem);

void abb_limpar(Abb *a);
int  abb_tamanho(const Abb *a);
int  abb_altura(const Abb *a);

/* ---- invariantes, para os testes. Não instrumentam. -------------------- */

/* Verdadeiro se o percurso em ordem sai crescente — a definição de ABB. */
int  abb_ordenada(const Abb *a);

/* Copia o percurso em ordem para `saida`, no máximo `max` valores, e devolve
 * quantos copiou. É o que permite comparar a árvore com um modelo de
 * referência sem o teste conhecer a estrutura por dentro. */
int  abb_em_ordem(const Abb *a, elem_t *saida, int max);

/* ---- AVL ---------------------------------------------------------------
 *
 * A mesma ABB com uma promessa a mais: para todo nó, |FB| <= 1, onde o fator
 * de balanceamento é a altura da subárvore esquerda menos a da direita. Manter
 * essa promessa custa uma rotação de vez em quando, e dá em troca a altura
 * O(log n) garantida — que é a única coisa que a ABB não consegue prometer.
 *
 * A interface é idêntica à da ABB, de propósito: as duas entram pela mesma
 * família do vtable, e o modo comparar roda a mesma sequência nas duas. Inserir
 * 1, 2, 3, 4… lado a lado é o argumento inteiro de a AVL existir, e não precisa
 * de uma palavra de texto para ser feito.                                   */

typedef struct Avl Avl;

Avl *avl_criar(void);
void avl_destruir(Avl *a);

int  avl_inserir(Avl *a, elem_t valor);
int  avl_remover(Avl *a, elem_t valor);
int  avl_remover_menor(Avl *a, elem_t *saida);
int  avl_menor(const Avl *a, elem_t *saida);
int  avl_buscar(const Avl *a, elem_t valor, int *profundidade);
int  avl_percurso(const Avl *a, int ordem);

void avl_limpar(Avl *a);
int  avl_tamanho(const Avl *a);
int  avl_altura(const Avl *a);

/* Quantas rotações a árvore já fez. É o preço do equilíbrio, e é a métrica que
 * a ABB não tem — nela o número seria sempre zero. */
int  avl_rotacoes(const Avl *a);

/* ---- invariantes, para os testes. Não instrumentam. -------------------- */

int  avl_ordenada(const Avl *a);
int  avl_em_ordem(const Avl *a, elem_t *saida, int max);

/* A promessa da AVL: |FB| <= 1 em TODO nó, e a altura guardada em cada nó
 * batendo com a altura real da subárvore dele.
 *
 * As duas juntas, porque a segunda é onde o bug mora: uma rotação que esquece
 * de atualizar a altura deixa a árvore equilibrada de fato e mentindo sobre
 * si mesma, e o desequilíbrio só aparece dezenas de inserções depois. */
int  avl_equilibrada(const Avl *a);

#endif /* DS_ARVORE_H */
