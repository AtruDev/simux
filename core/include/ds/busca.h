/* core/include/ds/busca.h — o vetor ordenado, e as duas buscas sobre ele.
 *
 * Um TAD, duas implementações — a mesma forma da pilha e da fila, e pelo mesmo
 * motivo. Aqui, porém, o que muda entre as duas não é como o dado é guardado:
 * é como ele é PROCURADO. As duas mantêm exatamente o mesmo vetor ordenado, e
 * inserem do mesmo jeito. Só `buscar` difere.
 *
 * É o que torna o modo comparar da Fase 2 o melhor jeito de contar esta aula.
 * As duas rodam a mesma sequência, lado a lado, sobre vetores idênticos, e o
 * contador de comparações conta a história inteira: uma anda o vetor, a outra
 * corta pela metade. Com n = 16, são 16 contra 5.
 *
 * Manter o vetor ordenado na inserção não é detalhe de implementação — é o
 * preço da busca binária, e ele fica visível: inserir no meio desloca todo o
 * resto para a direita, uma escrita de cada vez. O aluno vê O(n) na inserção
 * pagando por O(log n) na busca, que é a troca que a estrutura de dados faz.
 * Por isso a busca sequencial também usa o vetor ordenado: se ela guardasse na
 * ordem de chegada, a comparação misturaria duas diferenças de uma vez. */

#ifndef DS_BUSCA_H
#define DS_BUSCA_H

#include "ds/tipos.h"

typedef struct VetorOrd VetorOrd;

VetorOrd *vetor_ord_criar(int capacidade);
void      vetor_ord_destruir(VetorOrd *v);

/* Insere mantendo a ordem, deslocando o que estiver à direita. Repetidos são
 * aceitos: o vetor é um multiconjunto, e proibir repetido esconderia o caso
 * em que a busca binária pode achar qualquer um dos iguais. */
int       vetor_ord_inserir(VetorOrd *v, elem_t valor);

/* Remove o primeiro elemento, que é o menor. Existe para a estrutura ter uma
 * remoção — nesta aula quem interessa é a busca. */
int       vetor_ord_remover(VetorOrd *v, elem_t *saida);
int       vetor_ord_menor(const VetorOrd *v, elem_t *saida);
void      vetor_ord_limpar(VetorOrd *v);

int       vetor_ord_tamanho(const VetorOrd *v);
int       vetor_ord_capacidade(const VetorOrd *v);

/* As duas buscas. Devolvem OK e a posição por ponteiro, ou ERR_NAO_ENCONTRADO.
 *
 * Com valores repetidos elas podem parar em posições diferentes — a sequencial
 * acha sempre o primeiro, a binária acha um qualquer. É verdade sobre os dois
 * algoritmos, e não bug: o fuzz compara se acharam, não onde. */
int       vetor_ord_buscar_seq(const VetorOrd *v, elem_t valor, int *pos);
int       vetor_ord_buscar_bin(const VetorOrd *v, elem_t valor, int *pos);

/* Invariante: o vetor está em ordem não decrescente. Não instrumenta. */
int       vetor_ord_ordenado(const VetorOrd *v);

#endif /* DS_BUSCA_H */
