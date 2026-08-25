/* tests/test_pilha.c — pilha encadeada, comportamento e invariantes. */

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/idmap.h"
#include "ds/pilha.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "runner.h"

void suite_pilha(void)
{
    PilhaEnc *p;
    elem_t    v;
    int       i;

    trace_set_enabled(0);   /* o que se testa aqui é a estrutura, não o trace */
    idmap_reset();

    CASO("pilha nova esta vazia");
    p = pilha_enc_criar();
    ASSERT_TRUE(p != NULL);
    ASSERT_EQ(pilha_enc_tamanho(p), 0);
    ASSERT_EQ(pilha_enc_pop(p, &v), ERR_VAZIA);
    ASSERT_EQ(pilha_enc_topo(p, &v), ERR_VAZIA);

    CASO("push e topo");
    ASSERT_EQ(pilha_enc_push(p, 42), OK);
    ASSERT_EQ(pilha_enc_tamanho(p), 1);
    ASSERT_EQ(pilha_enc_topo(p, &v), OK);
    ASSERT_EQ(v, 42);
    /* topo consulta, não remove */
    ASSERT_EQ(pilha_enc_tamanho(p), 1);

    CASO("a ordem de saida e a inversa da de entrada");
    pilha_enc_limpar(p);
    for (i = 1; i <= 5; i++) {
        ASSERT_EQ(pilha_enc_push(p, i * 10), OK);
    }
    ASSERT_EQ(pilha_enc_tamanho(p), 5);
    for (i = 5; i >= 1; i--) {
        ASSERT_EQ(pilha_enc_pop(p, &v), OK);
        ASSERT_EQ(v, i * 10);
    }
    ASSERT_EQ(pilha_enc_tamanho(p), 0);
    ASSERT_EQ(pilha_enc_pop(p, &v), ERR_VAZIA);

    CASO("limpar esvazia de qualquer tamanho");
    for (i = 0; i < 37; i++) {
        ASSERT_EQ(pilha_enc_push(p, i), OK);
    }
    pilha_enc_limpar(p);
    ASSERT_EQ(pilha_enc_tamanho(p), 0);
    pilha_enc_limpar(p);            /* idempotente */
    ASSERT_EQ(pilha_enc_tamanho(p), 0);

    CASO("valores repetidos e negativos passam intactos");
    ASSERT_EQ(pilha_enc_push(p, -7), OK);
    ASSERT_EQ(pilha_enc_push(p, -7), OK);
    ASSERT_EQ(pilha_enc_push(p, 0), OK);
    ASSERT_EQ(pilha_enc_pop(p, &v), OK);
    ASSERT_EQ(v, 0);
    ASSERT_EQ(pilha_enc_pop(p, &v), OK);
    ASSERT_EQ(v, -7);
    ASSERT_EQ(pilha_enc_pop(p, &v), OK);
    ASSERT_EQ(v, -7);

    pilha_enc_destruir(p);

    /* ------------------------------------------------------------------
     * Invariante de memória: todo nó registrado no idmap teve id_esquece()
     * no free. Se um push vazar, isto acusa — e é a mesma checagem que vai
     * valer para árvore B daqui a cinco fases.
     * ------------------------------------------------------------------ */
    CASO("nenhum no sobrevive ao destruir");
    idmap_reset();
    p = pilha_enc_criar();
    for (i = 0; i < 50; i++) {
        ASSERT_EQ(pilha_enc_push(p, i), OK);
    }
    ASSERT_EQ(idmap_vivos(), 50);
    for (i = 0; i < 20; i++) {
        ASSERT_EQ(pilha_enc_pop(p, &v), OK);
    }
    ASSERT_EQ(idmap_vivos(), 30);
    pilha_enc_destruir(p);
    ASSERT_EQ(idmap_vivos(), 0);

    /* ------------------------------------------------------------------
     * O trace conta a história certa. Aqui ele é ligado de novo, porque o
     * que se testa é justamente a instrumentação.
     * ------------------------------------------------------------------ */
    CASO("push emite no, aresta, ponteiro e contadores");
    idmap_reset();
    trace_set_enabled(1);
    p = pilha_enc_criar();
    trace_reset();
    ASSERT_EQ(pilha_enc_push(p, 99), OK);
    {
        const ev_t *e = trace_ptr();
        ASSERT_EQ(trace_len(), 5);
        ASSERT_EQ(e[0].kind, EV_NODE_NEW);
        ASSERT_EQ(e[0].b, 99);
        ASSERT_EQ(e[1].kind, EV_EDGE_SET);
        ASSERT_EQ(e[1].c, 0);           /* prox era NULL, e NULL vale 0 */
        ASSERT_EQ(e[2].kind, EV_PTR_SET);
        ASSERT_EQ(e[2].a, PTR_TOPO);
        ASSERT_EQ(e[2].b, e[0].a);      /* topo aponta para o nó recém-criado */
        ASSERT_EQ(e[3].kind, EV_COUNT);
        ASSERT_EQ(e[3].a, CNT_TAMANHO);
        ASSERT_EQ(e[3].b, 1);
        ASSERT_EQ(e[4].a, CNT_ALOCACOES);
        /* todo evento sabe de onde veio */
        ASSERT_EQ(e[0].src, SRC_PILHA_ENC);
        ASSERT_TRUE(e[0].line > 0);
    }

    CASO("o segundo push liga o novo no ao anterior");
    {
        const ev_t *e;
        int32_t     id_primeiro, id_segundo;

        e = trace_ptr();
        id_primeiro = e[0].a;

        trace_reset();
        ASSERT_EQ(pilha_enc_push(p, 100), OK);
        e = trace_ptr();
        id_segundo = e[0].a;

        ASSERT_TRUE(id_segundo != id_primeiro);
        ASSERT_EQ(e[1].kind, EV_EDGE_SET);
        ASSERT_EQ(e[1].a, id_segundo);
        ASSERT_EQ(e[1].c, id_primeiro);  /* novo->prox aponta para o antigo */
    }

    CASO("pop visita, religa o topo e libera");
    trace_reset();
    ASSERT_EQ(pilha_enc_pop(p, &v), OK);
    ASSERT_EQ(v, 100);
    {
        const ev_t *e = trace_ptr();
        ASSERT_EQ(trace_len(), 4);
        ASSERT_EQ(e[0].kind, EV_VISIT);
        ASSERT_EQ(e[1].kind, EV_PTR_SET);
        ASSERT_EQ(e[2].kind, EV_NODE_FREE);
        ASSERT_EQ(e[2].a, e[0].a);      /* libera o mesmo nó que visitou */
        ASSERT_EQ(e[3].b, -1);          /* tamanho diminui */
    }

    CASO("pop em pilha vazia emite mensagem e nao mexe em nada");
    pilha_enc_limpar(p);
    trace_reset();
    ASSERT_EQ(pilha_enc_pop(p, &v), ERR_VAZIA);
    ASSERT_EQ(trace_len(), 1);
    ASSERT_EQ(trace_ptr()[0].kind, EV_MSG);
    ASSERT_EQ(trace_ptr()[0].a, STR_PILHA_VAZIA);

    pilha_enc_destruir(p);
    idmap_reset();
    trace_reset();
}
