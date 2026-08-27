/* core/include/ds/arvore_b.h — árvore B.
 *
 * A pergunta que ela responde é outra. A ABB e a AVL otimizam COMPARAÇÕES,
 * porque em memória é isso que custa. A árvore B otimiza ACESSOS A DISCO,
 * porque um deles custa uns dez milhões de vezes mais que uma comparação — e
 * a partir daí toda a forma dela se explica sozinha.
 *
 * Se ler uma página é caro e comparar dentro dela é de graça, a coisa certa a
 * fazer é pôr MUITAS chaves em cada página e ter POUCOS níveis. Daí um nó
 * guardar até 2t-1 chaves em vez de uma, e daí a árvore ser larga e baixa em
 * vez de estreita e alta. Com t = 50, um milhão de chaves cabe em três níveis:
 * três acessos a disco contra os vinte de uma AVL.
 *
 * As duas operações que a aula gasta o quadro explicando são a DIVISÃO e a
 * FUSÃO, e as duas existem pelo mesmo motivo: a árvore B mantém todas as
 * folhas na MESMA profundidade, e a única forma de crescer sem quebrar isso é
 * crescer pela raiz. Um nó cheio se divide e empurra a chave do meio para
 * cima; quando isso chega à raiz, a árvore ganha um nível — e ganha o nível
 * inteiro de uma vez, em todos os ramos.
 *
 * A inserção divide na DESCIDA, e não na volta. É a versão do CLRS, e a razão
 * é de disco: dividir preventivamente todo nó cheio por onde se passa garante
 * que o pai sempre tem espaço para a chave que sobe, e a inserção inteira vira
 * uma passada só. A versão que divide na volta precisa manter o caminho aberto
 * para reler os pais — em disco, isso é reler páginas. */

#ifndef DS_ARVORE_B_H
#define DS_ARVORE_B_H

#include "ds/tipos.h"

/* Grau mínimo máximo. Um nó guarda até 2t-1 chaves, então este teto é o que
 * dimensiona a struct do nó. Oito dá nós de até quinze chaves, muito além do
 * que cabe numa tela — o valor existe para a struct ter tamanho fixo. */
#define ARVORE_B_T_MAX 8

typedef struct ArvoreB ArvoreB;

/* `t` é o grau mínimo, entre 2 e ARVORE_B_T_MAX. Todo nó que não é raiz tem
 * entre t-1 e 2t-1 chaves; a raiz pode ter menos, e só ela. */
ArvoreB *arvore_b_criar(int t);
void     arvore_b_destruir(ArvoreB *a);

int      arvore_b_inserir(ArvoreB *a, elem_t chave);
int      arvore_b_remover(ArvoreB *a, elem_t chave);

/* Devolve OK e o NÍVEL em que achou, ou ERR_NAO_ENCONTRADO. O nível é o
 * número de páginas lidas menos um — é a medida que importa aqui. */
int      arvore_b_buscar(const ArvoreB *a, elem_t chave, int *nivel);

void     arvore_b_limpar(ArvoreB *a);

int      arvore_b_tamanho(const ArvoreB *a);
int      arvore_b_altura(const ArvoreB *a);
int      arvore_b_grau(const ArvoreB *a);

/* Acessos a disco desde que a árvore foi criada. É a métrica da fase. */
long     arvore_b_leituras(const ArvoreB *a);
long     arvore_b_escritas(const ArvoreB *a);

/* ---- invariantes, para os testes. Não instrumentam. -------------------- */

int      arvore_b_contem(const ArvoreB *a, elem_t chave);
int      arvore_b_em_ordem(const ArvoreB *a, elem_t *saida, int max);

/* As três promessas da árvore B, verificadas juntas:
 *
 *   1. todas as folhas na MESMA profundidade;
 *   2. t-1 <= chaves <= 2t-1 em todo nó que não é raiz;
 *   3. as chaves de cada nó em ordem, e cada subárvore dentro da faixa que o
 *      pai delimita.
 *
 * A terceira é a que pega o erro sutil: uma divisão que sobe a chave errada
 * deixa a árvore com a forma certa e a busca quebrada. */
int      arvore_b_valida(const ArvoreB *a);

#endif /* DS_ARVORE_B_H */
