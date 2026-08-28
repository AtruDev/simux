/* tests/test_api.c — a fronteira, e os slots de sessão que o modo comparar usa.
 *
 * Aqui não se testa estrutura: isso é assunto de test_pilha e test_fila. O que
 * se testa é o despacho e o isolamento entre os slots — que uma operação num
 * slot não mexa no outro, e que fechar um não estrague os ids do vizinho. */

#include <stddef.h>

#include "ds/api.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/tipos.h"
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

        /* Quatro, e não dois: cada trilha do modo comparar precisa da sua
         * sessão viva ao mesmo tempo, e a maior família — a do hash — tem
         * quatro implementações. Com dois, comparar as três listas já falhava
         * com ERR_ARG_INVALIDO na abertura da terceira. */
        ASSERT_TRUE(ds_sessao_slots() >= 4);
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

    /* A sessão de ordenação não tem TAD atrás dela, e é o único lugar em que
     * `tad == NULL` não quer dizer "sessão fechada". Vale checar que ela não
     * atende as operações do vtable nem o contrário. */
    CASO("sessão de ordenação: só gerar e ordenar");
    {
        ASSERT_EQ(ds_sessao_nova(TIPO_ORDENACAO, 16), OK);
        ASSERT_EQ(ds_tipo_sessao(), TIPO_ORDENACAO);
        ASSERT_EQ(ds_capacidade(), 16);
        ASSERT_EQ(ds_tamanho(), 0);

        ASSERT_EQ(ds_call(OP_PUSH, 5, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);

        /* Ordenar antes de gerar não tem o que ordenar. */
        ASSERT_EQ(ds_call(OP_ORDENAR, ALG_BOLHA, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_VAZIA);

        ASSERT_EQ(ds_call(OP_GERAR, 8, DIST_INVERSO, 1), OK);
        ASSERT_EQ(ds_tamanho(), 8);
        ASSERT_EQ(conta(EV_ARR_INIT), 1);
        ASSERT_EQ(conta(EV_ARR_WRITE), 8);

        ASSERT_EQ(ds_call(OP_ORDENAR, ALG_BOLHA, 0, 0), OK);
        ASSERT_TRUE(conta(EV_ARR_SWAP) > 0);
        /* A varredura final vem de api.c, uma marca por célula. */
        ASSERT_TRUE(conta(EV_ARR_MARK) >= 8);
        ASSERT_EQ(conta(EV_MSG), 1);

        ASSERT_EQ(ds_call(OP_GERAR, 99, DIST_ALEATORIO, 1), -1);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);
        ASSERT_EQ(ds_call(OP_ORDENAR, 99, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);

        ds_sessao_fim();
    }

    /* A estrutura da aba 1 não atende as operações da aba 2, e é o vtable que
     * responde por isso — não um `if` espalhado. */
    CASO("uma pilha não sabe ordenar");
    {
        ASSERT_EQ(ds_sessao_nova(TIPO_PILHA_VET, 8), OK);
        ASSERT_EQ(ds_call(OP_ORDENAR, ALG_BOLHA, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);
        ds_sessao_fim();
    }

    CASO("o buffer de entrada alimenta a distribuição manual");
    {
        elem_t *entrada;
        int     i;

        ASSERT_TRUE(ds_buffer(0) == NULL);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);

        entrada = ds_buffer(4);
        ASSERT_TRUE(entrada != NULL);
        entrada[0] = 40;
        entrada[1] = 10;
        entrada[2] = 30;
        entrada[3] = 20;

        ASSERT_EQ(ds_sessao_nova(TIPO_ORDENACAO, 4), OK);
        ASSERT_EQ(ds_call(OP_GERAR, 4, DIST_MANUAL, 0), OK);

        /* Os valores chegaram pelo buffer, e não pelos quatro inteiros. */
        {
            const ev_t *evs = ds_trace_ptr();
            int32_t     n = ds_trace_len();
            int32_t     j;
            int         escritos[4];
            int         k = 0;

            for (j = 0; j < n && k < 4; j++) {
                if (evs[j].kind == EV_ARR_WRITE) escritos[k++] = evs[j].b;
            }
            ASSERT_EQ(k, 4);
            for (i = 0; i < 4; i++) {
                ASSERT_EQ(escritos[i], entrada[i]);
            }
        }

        ds_sessao_fim();
    }

    /* O modo empírico. O que ele mede tem que bater com a teoria, senão o
     * gráfico não vale nada — e é justamente o gráfico que é o argumento. */
    CASO("ds_bench mede sem gerar evento");
    {
        int32_t comparacoes;
        int32_t cem;
        int32_t duzentos;

        ASSERT_EQ(ds_sessao_nova(TIPO_ORDENACAO, 8), OK);
        ASSERT_EQ(ds_call(OP_GERAR, 8, DIST_ALEATORIO, 1), OK);
        ASSERT_TRUE(ds_trace_len() > 0);

        comparacoes = ds_bench(ALG_MERGE, 1024, DIST_ALEATORIO, 7);
        ASSERT_TRUE(comparacoes > 0);
        ASSERT_TRUE(ds_bench_escritas() > 0);
        /* Nenhum evento: é o ponto inteiro de desligar o trace. */
        ASSERT_EQ(ds_trace_len(), 0);

        /* A sessão da tela não foi tocada pelo bench. */
        ASSERT_EQ(ds_tamanho(), 8);

        /* Dobrar n num algoritmo quadrático quadruplica as comparações; num
         * n log n, pouco mais que dobra. A margem é larga de propósito — o
         * que se testa é a ORDEM, não o número. */
        cem = ds_bench(ALG_SELECAO, 100, DIST_ALEATORIO, 3);
        duzentos = ds_bench(ALG_SELECAO, 200, DIST_ALEATORIO, 3);
        ASSERT_TRUE(duzentos > 3 * cem);

        cem = ds_bench(ALG_MERGE, 100, DIST_ALEATORIO, 3);
        duzentos = ds_bench(ALG_MERGE, 200, DIST_ALEATORIO, 3);
        ASSERT_TRUE(duzentos < 3 * cem);

        /* A mesma semente mede a mesma coisa. */
        ASSERT_EQ(ds_bench(ALG_QUICK, 512, DIST_ALEATORIO, 9),
                  ds_bench(ALG_QUICK, 512, DIST_ALEATORIO, 9));

        ASSERT_EQ(ds_bench(99, 100, DIST_ALEATORIO, 1), -1);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);
        ASSERT_EQ(ds_bench(ALG_BOLHA, 0, DIST_ALEATORIO, 1), -1);
        ASSERT_EQ(ds_erro(), ERR_ARG_INVALIDO);

        ds_sessao_fim();
    }
    /* O defeito que ficou escondido até a família hash chegar.
     *
     * Cada trilha do modo comparar abre a SUA sessão e todas ficam vivas ao
     * mesmo tempo, porque as operações chegam uma a uma e todas as trilhas
     * recebem cada uma delas. Com dois slots, comparar as três listas já
     * falhava na abertura da terceira — e ninguém tinha comparado listas. */
    CASO("uma família inteira em cena, cada trilha no seu slot");
    {
        static const int32_t FAMILIA[4] = {
            TIPO_HASH_ENC, TIPO_HASH_LINEAR, TIPO_HASH_QUAD, TIPO_HASH_DUPLO
        };
        int32_t slot;

        ASSERT_TRUE(ds_sessao_slots() >= 4);

        for (slot = 0; slot < 4; slot++) {
            ASSERT_EQ(ds_sessao_slot(slot), OK);
            ASSERT_EQ(ds_sessao_nova(FAMILIA[slot], 8), OK);
        }

        /* Abrir a quarta não pode ter fechado as três primeiras. */
        for (slot = 0; slot < 4; slot++) {
            ASSERT_EQ(ds_sessao_slot(slot), OK);
            ASSERT_EQ(ds_tipo_sessao(), FAMILIA[slot]);
        }

        /* E a mesma operação corre em todas, que é o que o modo comparar faz. */
        for (slot = 0; slot < 4; slot++) {
            ASSERT_EQ(ds_sessao_slot(slot), OK);
            ASSERT_EQ(ds_call(OP_PUSH, 8, 0, 0), OK);
            ASSERT_EQ(ds_call(OP_PUSH, 16, 0, 0), OK);
            ASSERT_EQ(ds_tamanho(), 2);
        }

        for (slot = 0; slot < 4; slot++) {
            ASSERT_EQ(ds_sessao_slot(slot), OK);
            ds_sessao_fim();
        }
    }

    /* A tabela hash não tem "o primeiro" nem "o menor", e o ponteiro nulo no
     * vtable é quem responde por isso. */
    CASO("hash recusa as operações sem argumento");
    {
        ASSERT_EQ(ds_sessao_slot(0), OK);
        ASSERT_EQ(ds_sessao_nova(TIPO_HASH_ENC, 8), OK);

        ASSERT_EQ(ds_call(OP_POP, 0, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);
        ASSERT_EQ(ds_call(OP_TOPO, 0, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);
        ASSERT_EQ(ds_call(OP_PERCURSO, 0, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_OP_DESCONHECIDA);

        /* Mas as que ela tem funcionam. */
        ASSERT_EQ(ds_call(OP_PUSH, 42, 0, 0), OK);
        ASSERT_EQ(ds_call(OP_BUSCAR, 42, 0, 0), OK);
        ASSERT_EQ(ds_call(OP_REMOVER_VALOR, 42, 0, 0), OK);
        ASSERT_EQ(ds_call(OP_BUSCAR, 42, 0, 0), -1);
        ASSERT_EQ(ds_erro(), ERR_NAO_ENCONTRADO);

        ds_sessao_fim();
    }
}
