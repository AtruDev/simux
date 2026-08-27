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

#endif /* DS_ARVORE_H */
