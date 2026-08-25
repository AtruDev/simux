/* tests/test_fila.c — as duas filas, com atenção ao que só a circular tem. */

#include "ds/erros.h"
#include "ds/fila.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "runner.h"

static void suite_fila_enc(void)
{
    FilaEnc *f;
    elem_t   v;
    int      i;

    CASO("fila encadeada: FIFO, ao contrario da pilha");
    idmap_reset();
    f = fila_enc_criar();
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ(fila_enc_tamanho(f), 0);
    ASSERT_EQ(fila_enc_desenfileirar(f, &v), ERR_VAZIA);

    for (i = 1; i <= 5; i++) {
        ASSERT_EQ(fila_enc_enfileirar(f, i * 10), OK);
    }
    /* Sai na MESMA ordem em que entrou — é a diferença inteira para a pilha. */
    for (i = 1; i <= 5; i++) {
        ASSERT_EQ(fila_enc_desenfileirar(f, &v), OK);
        ASSERT_EQ(v, i * 10);
    }
    ASSERT_EQ(fila_enc_tamanho(f), 0);

    CASO("esvaziar e voltar a usar mantem a ordem");
    /* O erro clássico desta estrutura: desenfileirar o último e deixar `fim`
     * apontando para memória liberada. A próxima inserção escreveria nela. */
    ASSERT_EQ(fila_enc_enfileirar(f, 1), OK);
    ASSERT_EQ(fila_enc_desenfileirar(f, &v), OK);
    ASSERT_EQ(fila_enc_enfileirar(f, 2), OK);
    ASSERT_EQ(fila_enc_enfileirar(f, 3), OK);
    ASSERT_EQ(fila_enc_frente(f, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(fila_enc_desenfileirar(f, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(fila_enc_desenfileirar(f, &v), OK);
    ASSERT_EQ(v, 3);

    CASO("fila encadeada nao vaza");
    idmap_reset();
    for (i = 0; i < 40; i++) {
        ASSERT_EQ(fila_enc_enfileirar(f, i), OK);
    }
    ASSERT_EQ(idmap_vivos(), 40);
    fila_enc_destruir(f);
    ASSERT_EQ(idmap_vivos(), 0);
}

static void suite_fila_vet(void)
{
    FilaVet *f;
    elem_t   v;
    int      i;

    CASO("fila com vetor: capacidade e overflow");
    f = fila_vet_criar(4);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ(fila_vet_capacidade(f), 4);
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), ERR_VAZIA);

    for (i = 1; i <= 4; i++) {
        ASSERT_EQ(fila_vet_enfileirar(f, i), OK);
    }
    ASSERT_EQ(fila_vet_tamanho(f), 4);
    /* A encadeada não teria como falhar aqui. É o que esta ensina. */
    ASSERT_EQ(fila_vet_enfileirar(f, 5), ERR_CHEIA);
    ASSERT_EQ(fila_vet_tamanho(f), 4);

    CASO("dar a volta preserva a ordem logica");
    /* Tira dois da frente e põe dois no fim: o fim passa do último índice e
     * volta ao começo, então a fila fica fisicamente partida em duas. A ordem
     * lógica não pode notar. */
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
    ASSERT_EQ(v, 1);
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(fila_vet_enfileirar(f, 5), OK);
    ASSERT_EQ(fila_vet_enfileirar(f, 6), OK);
    ASSERT_EQ(fila_vet_tamanho(f), 4);

    ASSERT_EQ(fila_vet_frente(f, &v), OK);
    ASSERT_EQ(v, 3);
    for (i = 3; i <= 6; i++) {
        ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(fila_vet_tamanho(f), 0);

    CASO("cheia e vazia nao se confundem");
    /* Com fim + 1 == inicio nos dois casos, só o contador separa. */
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(fila_vet_enfileirar(f, i), OK);
    }
    ASSERT_EQ(fila_vet_enfileirar(f, 99), ERR_CHEIA);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
    }
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), ERR_VAZIA);
    ASSERT_EQ(fila_vet_tamanho(f), 0);

    CASO("muitas voltas seguidas nao perdem nada");
    for (i = 0; i < 200; i++) {
        ASSERT_EQ(fila_vet_enfileirar(f, i), OK);
        ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(fila_vet_tamanho(f), 0);

    CASO("limpar zera de qualquer estado, inclusive dando a volta");
    for (i = 0; i < 3; i++) {
        ASSERT_EQ(fila_vet_enfileirar(f, i), OK);
    }
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), OK);
    ASSERT_EQ(fila_vet_enfileirar(f, 9), OK);
    fila_vet_limpar(f);
    ASSERT_EQ(fila_vet_tamanho(f), 0);
    ASSERT_EQ(fila_vet_desenfileirar(f, &v), ERR_VAZIA);
    /* e continua utilizável depois */
    ASSERT_EQ(fila_vet_enfileirar(f, 7), OK);
    ASSERT_EQ(fila_vet_frente(f, &v), OK);
    ASSERT_EQ(v, 7);

    fila_vet_destruir(f);
}

static void suite_fila_trace(void)
{
    FilaVet *f;
    elem_t   v;
    int      i;
    int      achou_volta;
    int32_t  k;

    CASO("a circular avisa quando o indice da a volta");
    trace_set_enabled(1);
    f = fila_vet_criar(3);
    for (i = 0; i < 3; i++) {
        (void) fila_vet_enfileirar(f, i);
    }
    (void) fila_vet_desenfileirar(f, &v);

    trace_reset();
    (void) fila_vet_enfileirar(f, 99);   /* este dá a volta: fim 2 -> 0 */

    achou_volta = 0;
    for (k = 0; k < trace_len(); k++) {
        const ev_t *e = &trace_ptr()[k];
        if (e->kind == EV_MSG && e->a == STR_DEU_VOLTA) achou_volta = 1;
    }
    ASSERT_TRUE(achou_volta);
    /* e escreveu no índice 0, não no 3 */
    ASSERT_EQ(trace_ptr()[1].kind, EV_ARR_WRITE);
    ASSERT_EQ(trace_ptr()[1].a, 0);

    CASO("o ponteiro da fila com vetor carrega indice, nao id");
    /* -1 é "nenhum" no mundo do vetor, porque 0 é um índice válido. */
    trace_reset();
    fila_vet_limpar(f);
    {
        int viu_menos_um = 0;
        for (k = 0; k < trace_len(); k++) {
            const ev_t *e = &trace_ptr()[k];
            if (e->kind == EV_PTR_SET && e->a == PTR_FRENTE && e->b == -1) {
                viu_menos_um = 1;
            }
        }
        ASSERT_TRUE(viu_menos_um);
    }

    fila_vet_destruir(f);
    trace_reset();
}

void suite_fila(void)
{
    trace_set_enabled(0);
    suite_fila_enc();
    suite_fila_vet();
    suite_fila_trace();
    trace_set_enabled(1);
    trace_reset();
    idmap_reset();
}
