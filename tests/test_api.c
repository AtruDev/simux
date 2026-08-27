/* tests/test_api.c — a fronteira, e os slots de sessão que o modo comparar usa.
 *
 * Aqui não se testa estrutura: isso é assunto de test_pilha e test_fila. O que
 * se testa é o despacho e o isolamento entre os slots — que uma operação num
 * slot não mexa no outro, e que fechar um não estrague os ids do vizinho. */

#include "ds/api.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "runner.h"

/* Quantos eventos EV_NODE_NEW o último trace emitiu, e o id do primeiro. */
static int32_t primeiro_no_criado(void)
{
    const ev_t *evs = ds_trace_ptr();
    int32_t     n = ds_trace_len();
    int32_t     i;

    for (i = 0; i < n; i++) {
        if (evs[i].kind == EV_NODE_NEW) {
            return evs[i].a;
        }
    }
    return 0;
}

static int32_t conta(int32_t kind)
{
    const ev_t *evs = ds_trace_ptr();
    int32_t     n = ds_trace_len();
    int32_t     i, total = 0;

    for (i = 0; i < n; i++) {
        if (evs[i].kind == kind) {
            total++;
        }
    }
    return total;
}

void suite_api(void)
{
    CASO("slot inválido é recusado, e não troca o slot ativo");
    {
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_sessao_slot(-1), -1);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);
        ASSERT_EQ(ds_sessao_slot(ds_sessao_slots()), -1);
        ASSERT_TRUE(ds_sessao_slots() >= 2);
    }

    CASO("cada slot guarda a sua própria estrutura");
    {
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_ENC, 8), OK);
        ASSERT_EQ(ds_sessao_slot(1), OK);
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_VET, 8), OK);

        /* Abrir o slot 1 não pode ter fechado o 0 — era o que a versão de uma
         * sessão só fazia, e é o defeito que o modo comparar encontraria. */
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_tipo_sessao(), TIPO_PILHA_ENC);
        ASSERT_EQ(ds_capacidade(), -1);   /* encadeada não tem limite */

        ASSERT_EQ(ds_sessao_slot(1), OK);
        ASSERT_EQ(ds_tipo_sessao(), TIPO_PILHA_VET);
        ASSERT_EQ(ds_capacidade(), 8);
    }

    CASO("a mesma operação nos dois slots conta histórias diferentes");
    {
        /* É o "pronto quando" do modo comparar: a encadeada aloca, a com vetor
         * escreve numa célula. Mesma sequência, custos diferentes. */
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_call(OP_PUSH, 7, 0, 0), OK);
        ASSERT_EQ(conta(EV_NODE_NEW), 1);
        ASSERT_EQ(conta(EV_ARR_WRITE), 0);

        ASSERT_EQ(ds_sessao_slot(1), OK);
        ASSERT_EQ(ds_call(OP_PUSH, 7, 0, 0), OK);
        ASSERT_EQ(conta(EV_NODE_NEW), 0);
        ASSERT_EQ(conta(EV_ARR_WRITE), 1);
    }

    CASO("operar num slot não mexe no outro");
    {
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_call(OP_POP, 0, 0, 0), OK);   /* esvazia o slot 0 */
        ASSERT_EQ(ds_call(OP_POP, 0, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_VAZIA);

        /* O slot 1 continua com o 7 que empilhou. */
        ASSERT_EQ(ds_sessao_slot(1), OK);
        ASSERT_EQ(ds_call(OP_TOPO, 0, 0, 0), OK);
    }

    CASO("fechar um slot não renumera os nós do outro");
    {
        int32_t id_antes, id_depois;

        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_ENC, 8), OK);
        ASSERT_EQ(ds_call(OP_PUSH, 1, 0, 0), OK);
        id_antes = primeiro_no_criado();
        ASSERT_TRUE(id_antes != 0);

        /* Fecha o slot 1, que estava aberto com a pilha com vetor. Se o
         * idmap fosse zerado aqui, o nó vivo do slot 0 receberia um id novo
         * na próxima vez, e o frontend desenharia sobre o nó errado. */
        ASSERT_EQ(ds_sessao_slot(1), OK);
        ds_sessao_fim();

        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_call(OP_PUSH, 2, 0, 0), OK);
        id_depois = primeiro_no_criado();

        /* Crescente, não reiniciado. "diferente" passaria mesmo com o reset,
         * que é justamente o que este caso existe para pegar. */
        ASSERT_TRUE(id_depois > id_antes);
    }

    CASO("com todos os slots fechados a numeração recomeça");
    {
        int32_t i;

        for (i = 0; i < ds_sessao_slots(); i++) {
            ASSERT_EQ(ds_sessao_slot(i), OK);
            ds_sessao_fim();
        }

        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_ENC, 8), OK);
        ASSERT_EQ(ds_call(OP_PUSH, 9, 0, 0), OK);
        ASSERT_EQ(primeiro_no_criado(), 1);
    }

    CASO("sem sessão, ds_call recusa — menos o ping");
    {
        ds_sessao_fim();
        ASSERT_EQ(ds_call(OP_PUSH, 1, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_SEM_SESSAO);
        ASSERT_EQ(ds_call(OP_PING, 0, 0, 0), OK);
        ASSERT_EQ(conta(EV_MSG), 1);
    }

    CASO("operação desconhecida");
    {
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_ENC, 8), OK);
        ASSERT_EQ(ds_call(9999, 0, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);
        ds_sessao_fim();
    }
}
