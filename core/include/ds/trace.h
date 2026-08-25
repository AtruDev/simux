/* core/include/ds/trace.h — o buffer de eventos.
 *
 * O core executa a operação e emite os micro-passos que aconteceram. Quem
 * anima é o frontend, na velocidade que quiser. Aqui não existe coordenada,
 * cor nem texto: só inteiros. */

#ifndef DS_TRACE_H
#define DS_TRACE_H

#include <stdint.h>

#include "ds/ids.h"

/* Um evento. Tamanho fixo, sem ponteiro e sem string, para o JS ler o buffer
 * inteiro como Int32Array — sem cópia e sem parse. */
typedef struct {
    int32_t kind;       /* ev_kind                                          */
    int32_t src;        /* ds_src — qual .c emitiu                          */
    int32_t line;       /* __LINE__ dentro daquele .c                       */
    int32_t a, b, c;    /* operandos; o significado depende de kind         */
} ev_t;

/* Inteiros por evento. O frontend usa isto como passo ao varrer o buffer;
 * é o único lugar onde o número 6 devia aparecer. */
#define EV_CAMPOS 6

/* Capacidade do buffer: 262144 eventos, cerca de 6 MB. Um quicksort de
 * n grande estoura isso — daí a flag de truncado, que a interface mostra. */
#define TRACE_CAP (1 << 18)

void        trace_reset(void);
void        trace_push(ev_t e);
int32_t     trace_len(void);
const ev_t *trace_ptr(void);
int32_t     trace_truncado(void);

/* Desligar o trace é o que permite medir comparações para n = 100 000 sem
 * gerar milhões de eventos. */
void        trace_set_enabled(int ligado);
int         trace_enabled(void);

/* A macro que instrumenta o algoritmo.
 *
 * Cada .c instrumentado abre com `#define TR_SRC SRC_<ARQUIVO>`; a linha
 * entra sozinha. Sempre passe ao menos um campo — todo evento do
 * vocabulário carrega pelo menos `a`:
 *
 *     TR(EV_ARR_COMPARE, .a = j, .b = j + 1);
 *
 * Os campos não citados ficam zerados, por serem inicializadores
 * designados. */
#define TR(k, ...) \
    trace_push((ev_t){ .kind = (k), .src = TR_SRC, .line = __LINE__, __VA_ARGS__ })

#endif /* DS_TRACE_H */
