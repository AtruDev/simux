/* cli/main.c — binário de terminal sobre o mesmo core que vai para o wasm.
 *
 * Serve de ferramenta de depuração: roda uma sequência de operações e despeja
 * o trace em texto, sem navegador no meio. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds/api.h"
#include "ds/erros.h"
#include "ds/ids.h"

static const char *NOME_EVENTO[] = {
    "MSG", "COUNT", "PHASE",
    "ARR_INIT", "ARR_READ", "ARR_COMPARE", "ARR_SWAP", "ARR_WRITE",
    "ARR_RANGE", "ARR_MARK", "AUX_INIT", "AUX_WRITE",
    "NODE_NEW", "NODE_FREE", "NODE_SET", "EDGE_SET", "PTR_SET",
    "VISIT", "UNVISIT", "DISK_READ", "DISK_WRITE",
};

static void despejar(const char *rotulo)
{
    const ev_t *eventos = ds_trace_ptr();
    int32_t     n = ds_trace_len();
    int32_t     i;

    printf("%-14s %2d evento(s)%s\n", rotulo, (int) n,
           ds_trace_truncado() ? "  (truncado)" : "");

    for (i = 0; i < n; i++) {
        const ev_t *e = &eventos[i];
        const char *nome = (e->kind >= 0 && e->kind < EV_KIND_COUNT)
                           ? NOME_EVENTO[e->kind] : "?";

        printf("    %-11s src=%d linha=%-4d a=%-4d b=%-4d c=%d\n",
               nome, (int) e->src, (int) e->line,
               (int) e->a, (int) e->b, (int) e->c);
    }
}

int main(int argc, char **argv)
{
    int i;

    printf("simux %s\n\n", SIMUX_VERSAO);

    if (ds_sessao_nova(TIPO_PILHA_ENC) != OK) {
        fprintf(stderr, "não foi possível abrir a sessão: %d\n",
                (int) ds_erro());
        return 1;
    }

    /* Sem argumentos, roda uma cena curta. Com argumentos, empilha cada um. */
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            char rotulo[32];
            snprintf(rotulo, sizeof rotulo, "push %s", argv[i]);
            ds_call(OP_PUSH, atoi(argv[i]), 0, 0);
            despejar(rotulo);
        }
    } else {
        ds_call(OP_PUSH, 10, 0, 0);  despejar("push 10");
        ds_call(OP_PUSH, 20, 0, 0);  despejar("push 20");
        ds_call(OP_TOPO, 0, 0, 0);   despejar("topo");
        ds_call(OP_POP, 0, 0, 0);    despejar("pop");
        ds_call(OP_LIMPAR, 0, 0, 0); despejar("limpar");
        ds_call(OP_POP, 0, 0, 0);    despejar("pop (vazia)");
    }

    ds_sessao_fim();
    return 0;
}
