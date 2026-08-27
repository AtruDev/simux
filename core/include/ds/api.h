/* core/include/ds/api.h — a fronteira com o mundo de fora.
 *
 * É a mesma API para o wasm e para o CLI. As declarações aqui são C puro:
 * quem sabe da existência do Emscripten é core/api/api.c, e só ele.
 *
 * A fronteira inteira são inteiros. Nenhuma string atravessa: EV_MSG carrega
 * um id STR_*, e a frase vive no i18n do frontend. */

#ifndef DS_API_H
#define DS_API_H

#include <stdint.h>

#include "ds/tipos.h"
#include "ds/trace.h"

/* Abre uma sessão sobre uma estrutura (TIPO_* em ids.h), descartando a
 * anterior. Devolve OK, ou -1 e o motivo em ds_erro().
 *
 * O plano declarava isto como void e sem capacidade. Devolver status segue a
 * regra de que toda função pública do projeto devolve status, e a criação pode
 * faltar memória; a capacidade é necessária desde que existem implementações
 * com vetor, e é ignorada pelas encadeadas. */
int32_t     ds_sessao_nova(int32_t tipo, int32_t capacidade);
void        ds_sessao_fim(void);
int32_t     ds_tipo_sessao(void);

/* Escolhe o slot de sessão sobre o qual as próximas chamadas operam, e quantos
 * existem. Devolve OK, ou -1 e ERR_ARG_INVALIDO em ds_erro().
 *
 * Existem porque o modo comparar precisa das duas implementações do mesmo TAD
 * vivas ao mesmo tempo, alimentadas pela mesma sequência. O slot fica fora do
 * ds_call de propósito: ele muda uma vez por operação, não a cada chamada, e a
 * fronteira continua sendo quatro inteiros. */
int32_t     ds_sessao_slot(int32_t slot);
int32_t     ds_sessao_slots(void);

/* Quantos elementos a estrutura da sessão tem agora.
 *
 * O modelo do frontend também sabe um tamanho, mas o dele é o do instante em
 * que a animação está. Este é o de verdade. */
int32_t     ds_tamanho(void);

/* Capacidade da estrutura da sessão, ou -1 quando não há limite. */
int32_t     ds_capacidade(void);

/* Executa uma operação. Devolve OK, ou -1 e o motivo em ds_erro().
 * O trace é zerado no início de cada chamada. */
int32_t     ds_call(int32_t op, int32_t a, int32_t b, int32_t c);

/* Código do último erro (ds_erro em erros.h). */
int32_t     ds_erro(void);

/* Leitura do trace, sem cópia: o JS monta um Int32Array sobre este ponteiro.
 *
 * A view precisa ser recriada depois de toda chamada ao wasm — com
 * ALLOW_MEMORY_GROWTH, qualquer malloc que cresça a heap desanexa as views
 * antigas, e guardar uma em variável de módulo é bug garantido. */
const ev_t *ds_trace_ptr(void);
int32_t     ds_trace_len(void);
int32_t     ds_trace_truncado(void);

/* Buffer de ENTRADA, para o dado que não cabe nos quatro inteiros de ds_call.
 *
 * Devolve um ponteiro para dentro da heap, ou NULL com o motivo em ds_erro().
 * Hoje serve à distribuição manual da aba de ordenação: o JS escreve n
 * valores aqui e chama OP_GERAR com DIST_MANUAL. Continua sem string e sem
 * parser — a regra é que nenhum texto atravessa, não que nenhum dado atravessa.
 *
 * Como qualquer chamada que pode alocar, esta pode crescer a heap e desanexar
 * as views antigas: recrie a view depois dela também. */
elem_t     *ds_buffer(int32_t n);

/* Modo empírico: roda um algoritmo com o trace DESLIGADO e devolve quantas
 * comparações fez, ou -1 e o motivo em ds_erro().
 *
 * É o que separa "eu implementei os algoritmos" de "eu medi, e a curva bate
 * com a teoria". Com o trace ligado, medir n = 25 600 geraria centenas de
 * milhões de eventos para ninguém assistir. */
int32_t     ds_bench(int32_t alg, int32_t n, int32_t dist, int32_t semente);
int32_t     ds_bench_escritas(void);

#endif /* DS_API_H */
