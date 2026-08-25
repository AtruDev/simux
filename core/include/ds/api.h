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

#include "ds/trace.h"

/* Abre uma sessão sobre uma estrutura (TIPO_* em ids.h), descartando a
 * anterior. Devolve OK, ou -1 e o motivo em ds_erro().
 *
 * O plano declarava isto como void; devolver status segue a regra de que toda
 * função pública do projeto devolve status, e a criação pode faltar memória. */
int32_t     ds_sessao_nova(int32_t tipo);
void        ds_sessao_fim(void);
int32_t     ds_tipo_sessao(void);

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

#endif /* DS_API_H */
