/* core/include/ds/arvore_bmais.h — árvore B+.
 *
 * A árvore B já respondeu a pergunta do disco: muitas chaves por página,
 * poucos níveis por busca. A B+ responde a SEGUNDA pergunta, a que o banco de
 * dados faz o dia inteiro e que a árvore B responde mal — *me dê todas as
 * chaves entre 300 e 900, em ordem*.
 *
 * Numa árvore B, ler tudo em ordem é um percurso: sobe e desce, e cada subida
 * relê uma página que já tinha sido lida. Numa B+, é uma varredura reta. Duas
 * decisões, e as duas saem daí:
 *
 *   1. TODA chave mora numa folha. O que fica nos nós internos é SEPARADOR —
 *      um roteiro, não um dado. Por isso a chave do meio de uma folha cheia é
 *      COPIADA para o pai em vez de subir: ela continua embaixo, onde os
 *      dados moram.
 *   2. AS FOLHAS SÃO ENCADEADAS, da menor para a maior. Achou a primeira
 *      chave da faixa, o resto é seguir ponteiro — sem tocar num nó interno,
 *      sem reler página nenhuma.
 *
 * O preço está na busca pontual, e é honesto mostrá-lo: a árvore B pode achar
 * a chave na raiz e parar; a B+ desce SEMPRE até a folha, porque é só lá que
 * existe dado. Uma busca custa exatamente a altura, todas as vezes. Trocou-se
 * o melhor caso da busca por uma varredura barata — e é por isso que quase
 * todo índice de banco de dados é B+, e não B: uma consulta que devolve faixa
 * é mais comum que uma que devolve uma linha.
 *
 * O separador que sobra é a consequência que assusta na primeira leitura, e
 * está certa: removida a chave 42, o separador 42 lá em cima CONTINUA. Ele
 * nunca foi um dado — só diz "42 e acima estão à direita", e isso continua
 * verdade. A busca desce até a folha de qualquer jeito e não acha nada. */

#ifndef DS_ARVORE_BMAIS_H
#define DS_ARVORE_BMAIS_H

#include "ds/tipos.h"

/* Mesmo teto da árvore B, e pela mesma razão: um nó guarda até 2t-1 chaves,
 * e é este limite que dimensiona a struct. Repetido aqui, e não importado de
 * arvore_b.h, para as duas estruturas continuarem independentes — elas se
 * parecem, mas nenhuma é implementada em termos da outra. */
#define ARVORE_BMAIS_T_MAX 8

typedef struct ArvoreBMais ArvoreBMais;

/* `t` é o grau mínimo, entre 2 e ARVORE_BMAIS_T_MAX. */
ArvoreBMais *arvore_bmais_criar(int t);
void         arvore_bmais_destruir(ArvoreBMais *a);

int          arvore_bmais_inserir(ArvoreBMais *a, elem_t chave);
int          arvore_bmais_remover(ArvoreBMais *a, elem_t chave);

/* Devolve OK e a PROFUNDIDADE da folha em que parou. Ela é sempre a mesma —
 * a altura menos um —, e é esse o ponto: aqui não existe achar cedo. */
int          arvore_bmais_buscar(const ArvoreBMais *a, elem_t chave, int *nivel);

/* A varredura sequencial, que é a razão de a estrutura existir: começa na
 * folha mais à esquerda e segue os elos até o fim, uma página lida por folha
 * e nenhum nó interno tocado.
 *
 * Compare com arvore_b_varrer sobre o mesmo conjunto: a árvore B lê as
 * páginas internas também, e relê as de cima a cada subida. */
int          arvore_bmais_varrer(const ArvoreBMais *a);

void         arvore_bmais_limpar(ArvoreBMais *a);

int          arvore_bmais_tamanho(const ArvoreBMais *a);
int          arvore_bmais_altura(const ArvoreBMais *a);
int          arvore_bmais_grau(const ArvoreBMais *a);
/* Quantas folhas — o custo, em páginas, de uma varredura completa. */
int          arvore_bmais_folhas(const ArvoreBMais *a);

long         arvore_bmais_leituras(const ArvoreBMais *a);
long         arvore_bmais_escritas(const ArvoreBMais *a);

/* ---- invariantes, para os testes. Não instrumentam. -------------------- */

int          arvore_bmais_contem(const ArvoreBMais *a, elem_t chave);

/* Lê pelo ELO entre as folhas, e não descendo a árvore. É de propósito: é a
 * leitura que a estrutura promete, e comparar o que ela devolve com o que a
 * descida encontra é o que denuncia um elo remendado errado numa divisão ou
 * esquecido numa fusão. */
int          arvore_bmais_em_ordem(const ArvoreBMais *a, elem_t *saida, int max);

/* As promessas da árvore B, mais as duas que são só da B+:
 *
 *   1. todas as folhas na MESMA profundidade;
 *   2. t-1 <= chaves <= 2t-1 em todo nó que não é raiz;
 *   3. as chaves em ordem, e cada subárvore dentro da faixa que o pai
 *      delimita — aqui com o separador incluído à DIREITA, que é a regra de
 *      roteamento de uma B+;
 *   4. o dado está SÓ nas folhas: o tamanho é a soma das chaves das folhas, e
 *      as chaves dos nós internos não entram na conta — elas são separadores,
 *      e um separador pode nomear uma chave que já foi removida;
 *   5. a corrente de folhas passa por TODAS elas, na ordem crescente, e
 *      devolve exatamente as mesmas chaves que a descida encontra.
 *
 * A quinta é a que pega o erro que só a B+ tem: uma fusão que esquece de
 * remendar o elo deixa a árvore perfeita para a busca e cega para a
 * varredura — e é justamente a varredura que a estrutura existe para fazer. */
int          arvore_bmais_valida(const ArvoreBMais *a);

#endif /* DS_ARVORE_BMAIS_H */
