/* core/api/api.c — o único arquivo do core que conhece o Emscripten.
 *
 * Todo o resto compila igual nativo e para wasm, o que dá testes rápidos,
 * gdb e um binário de terminal de graça. */

#define TR_SRC SRC_API

#include "ds/api.h"

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif

static int32_t g_erro = OK;

API int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c)
{
    /* Ainda não há operação que use os operandos; eles existem na assinatura
     * porque é ela que evita string na fronteira quando as estruturas
     * chegarem. */
    (void) a;
    (void) b;
    (void) c;

    trace_reset();
    g_erro = OK;

    switch (op) {
    case OP_PING:
        TR(EV_MSG, .a = STR_PING);
        return OK;

    default:
        g_erro = ERR_OP_DESCONHECIDA;
        return -1;
    }
}

API int32_t ds_erro(void)
{
    return g_erro;
}

API const ev_t *ds_trace_ptr(void)
{
    return trace_ptr();
}

API int32_t ds_trace_len(void)
{
    return trace_len();
}

API int32_t ds_trace_truncado(void)
{
    return trace_truncado();
}
