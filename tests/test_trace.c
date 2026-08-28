/* tests/test_trace.c — o buffer de eventos. */

#define TR_SRC SRC_API

#include <stddef.h>

#include "ds/ids.h"
#include "ds/trace.h"

#include "runner.h"

/* Emite um evento e devolve a linha em que o TR está. Precisa ser uma função
 * de uma linha só para a conferência de __LINE__ ficar honesta. */
static int emitir_marcado(void)
{
    TR(EV_ARR_COMPARE, .a = 7, .b = 9); return __LINE__;
}

void suite_trace(void)
{
    int32_t     i;
    const ev_t *v;

    CASO("o evento tem o tamanho que o Int32Array do frontend assume");
    /* Se isto falhar, o frontend passa a ler o buffer com o passo errado e
     * todo evento sai embaralhado — vale travar aqui. */
    ASSERT_EQ(sizeof(ev_t), (size_t) EV_CAMPOS * sizeof(int32_t));

    CASO("reset esvazia");
    trace_reset();
    ASSERT_EQ(trace_len(), 0);
    ASSERT_EQ(trace_truncado(), 0);

    CASO("push guarda os campos como vieram");
    trace_reset();
    trace_push((ev_t){ .kind = EV_NODE_NEW, .src = SRC_API, .line = 123,
                       .a = 4, .b = 5, .c = 6 });
    ASSERT_EQ(trace_len(), 1);
    v = trace_ptr();
    ASSERT_EQ(v[0].kind, EV_NODE_NEW);
    ASSERT_EQ(v[0].src, SRC_API);
    ASSERT_EQ(v[0].line, 123);
    ASSERT_EQ(v[0].a, 4);
    ASSERT_EQ(v[0].b, 5);
    ASSERT_EQ(v[0].c, 6);

    CASO("TR preenche src e a linha sozinho");
    trace_reset();
    {
        int linha = emitir_marcado();
        v = trace_ptr();
        ASSERT_EQ(trace_len(), 1);
        ASSERT_EQ(v[0].kind, EV_ARR_COMPARE);
        ASSERT_EQ(v[0].src, SRC_API);
        ASSERT_EQ(v[0].line, linha);
        ASSERT_EQ(v[0].a, 7);
        ASSERT_EQ(v[0].b, 9);
        /* campo não citado fica zerado */
        ASSERT_EQ(v[0].c, 0);
    }

    CASO("desligado, push nao grava");
    trace_reset();
    trace_set_enabled(0);
    ASSERT_EQ(trace_enabled(), 0);
    TR(EV_MSG, .a = STR_PING);
    ASSERT_EQ(trace_len(), 0);
    trace_set_enabled(1);
    ASSERT_EQ(trace_enabled(), 1);
    TR(EV_MSG, .a = STR_PING);
    ASSERT_EQ(trace_len(), 1);

    CASO("encher marca truncado sem passar da capacidade");
    trace_reset();
    for (i = 0; i < (int32_t) TRACE_CAP; i++) {
        TR(EV_ARR_READ, .a = i);
    }
    ASSERT_EQ(trace_len(), (int32_t) TRACE_CAP);
    ASSERT_EQ(trace_truncado(), 0);

    TR(EV_ARR_READ, .a = 0);
    ASSERT_EQ(trace_len(), (int32_t) TRACE_CAP);
    ASSERT_EQ(trace_truncado(), 1);

    /* e o reset limpa a marca */
    trace_reset();
    ASSERT_EQ(trace_truncado(), 0);
}
