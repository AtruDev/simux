/* core/include/ds/ordenacao.h — a aba 2 vista de fora.
 *
 * Um algoritmo de ordenação aqui é uma função só: recebe o vetor e o tamanho,
 * ordena no lugar, e emite pelo caminho os eventos do que fez. Nenhum deles
 * devolve nada além do status, porque o resultado É o vetor.
 *
 * A instrumentação não muda o algoritmo. `bolha_ordenar` é a bolha que a
 * matéria ensina, com a flag de parada e tudo; as chamadas TR ficam nos
 * pontos onde a aula pararia para explicar — a comparação, a troca, o fim de
 * uma passada. Se elas atrapalharem a leitura do laço, o problema é o
 * vocabulário de eventos, não o algoritmo (é o painel de código que exibe
 * este arquivo, e ele precisa continuar sendo aula).
 *
 * O mergesort é a única exceção à assinatura, por dentro: ele aloca o buffer
 * auxiliar uma vez e o passa na recursão. Alocar por chamada seria O(n log n)
 * mallocs, e o contador de alocações mentiria sobre o algoritmo. */

#ifndef DS_ORDENACAO_H
#define DS_ORDENACAO_H

#include "ds/tipos.h"

int bolha_ordenar(elem_t *v, int n);
int selecao_ordenar(elem_t *v, int n);
int insercao_ordenar(elem_t *v, int n);
int shell_ordenar(elem_t *v, int n);
int quick_ordenar(elem_t *v, int n);
int merge_ordenar(elem_t *v, int n);

/* A intercalação externa não ordena na memória: o vetor está "no disco", e o
 * que cabe na RAM são `k` registros. A assinatura é a mesma dos outros seis —
 * é o que a deixa entrar na tabela sem caso especial —, e o k vem por fora,
 * como a semente da cena vem. */
int externa_ordenar(elem_t *v, int n);

/* Quantos registros cabem na memória. Abaixo de 2 vira 2; acima de n, o
 * arquivo inteiro cabe na RAM e não sobra intercalação — que é um caso que
 * vale ver. É o `t` da árvore B desta aba: o único parâmetro, e o que muda o
 * número de passadas. */
void externa_memoria(int k);
int  externa_memoria_atual(void);

/* Do último externa_ordenar. Servem aos testes e ao painel; o frontend lê os
 * mesmos números pelo trace, como sempre. */
long externa_leituras(void);
long externa_escritas(void);
int  externa_passadas(void);

/* Despacho por ALG_* de ids.h. Devolve NULL para id fora da faixa — é o que
 * api.c testa para responder ERR_ARG_INVALIDO. */
typedef int (*OrdenaFn)(elem_t *v, int n);

OrdenaFn ordenacao_de(int alg);

/* ---- a cena: o vetor inicial ------------------------------------------- *
 *
 * Gerar a distribuição é parte do core, e não do frontend, por um motivo
 * único e suficiente: a mesma semente tem que dar o mesmo vetor em qualquer
 * máquina, e o Math.random do JS não tem semente. É o que faz um link
 * compartilhado abrir exatamente a mesma cena.                             */

/* Preenche v[0..n-1] segundo DIST_* e a semente, e emite EV_ARR_INIT mais as
 * escritas iniciais. `manual` só é lido em DIST_MANUAL, e pode ser NULL nas
 * outras. Devolve OK ou ERR_ARG_INVALIDO. */
int cena_gerar(elem_t *v, int n, int dist, unsigned int semente,
               const elem_t *manual);

/* Verdadeiro se v[0..n-1] está em ordem não decrescente. Não instrumenta:
 * serve aos testes e à checagem final do api.c. */
int cena_ordenado(const elem_t *v, int n);

/* Verdadeiro se `depois` é uma permutação de `antes`. É a metade da correção
 * que "está ordenado" não cobre: um algoritmo que zera o vetor inteiro passa
 * no teste de ordem e falha neste. */
int cena_permutacao(const elem_t *antes, const elem_t *depois, int n);

/* ---- medida ------------------------------------------------------------ *
 *
 * Comparações e escritas contadas do lado do C, e não deduzidas dos eventos,
 * por causa do modo empírico: ele roda com o trace desligado, justamente para
 * medir n = 25 600 sem gerar milhões de eventos. As mesmas macros de
 * core/sort/passos.h alimentam os dois — o contador e o trace saem da mesma
 * linha, então eles não têm como divergir.                                 */

void medida_zerar(void);
void medida_comparou(void);
void medida_escreveu(int quantas);
long medida_comparacoes(void);
long medida_escritas(void);

#endif /* DS_ORDENACAO_H */
