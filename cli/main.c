/* cli/main.c — binário de terminal sobre o mesmo core que vai para o wasm.
 *
 * Serve de ferramenta de depuração: despeja o trace de uma operação em texto,
 * sem navegador no meio. Cresce junto com as estruturas. */

#include <stdio.h>

#include "ds/api.h"
#include "ds/erros.h"
#include "ds/ids.h"

int main(void)
{
    const ev_t *eventos;
    int32_t     n, i;

    printf("simux %s\n", SIMUX_VERSAO);

    if (ds_call(OP_PING, 0, 0, 0) != OK) {
        fprintf(stderr, "ds_call(OP_PING) falhou com erro %d\n",
                (int) ds_erro());
        return 1;
    }

    eventos = ds_trace_ptr();
    n = ds_trace_len();

    printf("%d evento(s)%s\n", (int) n,
           ds_trace_truncado() ? " (truncado)" : "");

    for (i = 0; i < n; i++) {
        const ev_t *e = &eventos[i];
        printf("  [%d] kind=%d src=%d linha=%d a=%d b=%d c=%d\n",
               (int) i, (int) e->kind, (int) e->src, (int) e->line,
               (int) e->a, (int) e->b, (int) e->c);
    }

    return 0;
}
