/* tests/test_lista.c — as três listas, pela interface do vtable.
 *
 * Testar pelo vtable e não pelas funções concretas é de propósito: o mesmo
 * corpo de teste roda nas três, então nenhuma ganha um caso que as outras não
 * têm. O que é específico de uma implementação — o `ant` da dupla, o ciclo da
 * circular — fica nos casos de invariante no fim do arquivo. */

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/lista.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

/** Os valores da lista, em ordem, lidos por remoção destrutiva. */
static void esvaziar_em(const TAD_Linear *tad, void *l, elem_t *saida, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        saida[i] = 0;
        ASSERT_EQ(tad->remover_em(l, 0, &saida[i]), OK);
    }
}

/* Roda a bateria inteira numa implementação. */
static void bateria(const TAD_Linear *tad)
{
    void  *l = tad->criar(0);
    elem_t v = 0;
    elem_t saida[8];
    int    pos = -1;

    ASSERT_TRUE(l != NULL);

    CASO("lista nova está vazia");
    ASSERT_EQ(tad->tamanho(l), 0);
    ASSERT_EQ(tad->remover_em(l, 0, &v), ERR_VAZIA);
    ASSERT_EQ(tad->consultar(l, &v), ERR_VAZIA);
    ASSERT_EQ(tad->buscar(l, 1, &pos), ERR_NAO_ENCONTRADO);

    CASO("inserir no fim mantém a ordem de chegada");
    ASSERT_EQ(tad->inserir_em(l, 0, 10), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 20), OK);
    ASSERT_EQ(tad->inserir_em(l, 2, 30), OK);
    ASSERT_EQ(tad->tamanho(l), 3);
    esvaziar_em(tad, l, saida, 3);
    ASSERT_EQ(saida[0], 10);
    ASSERT_EQ(saida[1], 20);
    ASSERT_EQ(saida[2], 30);
    ASSERT_EQ(tad->tamanho(l), 0);

    CASO("inserir no início inverte");
    ASSERT_EQ(tad->inserir_em(l, 0, 10), OK);
    ASSERT_EQ(tad->inserir_em(l, 0, 20), OK);
    ASSERT_EQ(tad->inserir_em(l, 0, 30), OK);
    esvaziar_em(tad, l, saida, 3);
    ASSERT_EQ(saida[0], 30);
    ASSERT_EQ(saida[1], 20);
    ASSERT_EQ(saida[2], 10);

    CASO("inserir no meio entra entre os vizinhos");
    ASSERT_EQ(tad->inserir_em(l, 0, 1), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 3), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 2), OK);   /* 1, 2, 3 */
    esvaziar_em(tad, l, saida, 3);
    ASSERT_EQ(saida[0], 1);
    ASSERT_EQ(saida[1], 2);
    ASSERT_EQ(saida[2], 3);

    CASO("remover do meio costura os vizinhos");
    ASSERT_EQ(tad->inserir_em(l, 0, 1), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 2), OK);
    ASSERT_EQ(tad->inserir_em(l, 2, 3), OK);
    ASSERT_EQ(tad->remover_em(l, 1, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(tad->tamanho(l), 2);
    esvaziar_em(tad, l, saida, 2);
    ASSERT_EQ(saida[0], 1);
    ASSERT_EQ(saida[1], 3);

    CASO("remover a última ponta deixa a lista utilizável");
    /* É onde os ponteiros da estrutura precisam voltar ao estado de lista
     * vazia. Errar aqui não quebra na hora: quebra na inserção seguinte. */
    ASSERT_EQ(tad->inserir_em(l, 0, 7), OK);
    ASSERT_EQ(tad->remover_em(l, 0, &v), OK);
    ASSERT_EQ(v, 7);
    ASSERT_EQ(tad->tamanho(l), 0);
    ASSERT_EQ(tad->inserir_em(l, 0, 8), OK);
    ASSERT_EQ(tad->consultar(l, &v), OK);
    ASSERT_EQ(v, 8);
    tad->limpar(l);
    ASSERT_EQ(tad->tamanho(l), 0);

    CASO("posição fora da faixa é recusada, e nada muda");
    ASSERT_EQ(tad->inserir_em(l, 0, 5), OK);
    ASSERT_EQ(tad->inserir_em(l, -1, 9), ERR_ARG_INVALIDO);
    ASSERT_EQ(tad->inserir_em(l, 2, 9), ERR_ARG_INVALIDO);   /* n = 1 */
    ASSERT_EQ(tad->remover_em(l, 1, &v), ERR_ARG_INVALIDO);
    ASSERT_EQ(tad->remover_em(l, -1, &v), ERR_ARG_INVALIDO);
    ASSERT_EQ(tad->tamanho(l), 1);
    tad->limpar(l);

    CASO("buscar devolve a posição, e recusa o que não está lá");
    ASSERT_EQ(tad->inserir_em(l, 0, 10), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 20), OK);
    ASSERT_EQ(tad->inserir_em(l, 2, 30), OK);
    ASSERT_EQ(tad->buscar(l, 10, &pos), OK);
    ASSERT_EQ(pos, 0);
    ASSERT_EQ(tad->buscar(l, 30, &pos), OK);
    ASSERT_EQ(pos, 2);
    ASSERT_EQ(tad->buscar(l, 99, &pos), ERR_NAO_ENCONTRADO);

    CASO("buscar acha a PRIMEIRA ocorrência");
    tad->limpar(l);
    ASSERT_EQ(tad->inserir_em(l, 0, 4), OK);
    ASSERT_EQ(tad->inserir_em(l, 1, 4), OK);
    ASSERT_EQ(tad->buscar(l, 4, &pos), OK);
    ASSERT_EQ(pos, 0);

    CASO("as operações sem posição agem no início");
    /* É o contrato que a lista assina com o TAD_Linear, e o que o modo
     * comparar usa: inserir, remover e consultar na mesma ponta. */
    tad->limpar(l);
    ASSERT_EQ(tad->inserir(l, 1), OK);
    ASSERT_EQ(tad->inserir(l, 2), OK);
    ASSERT_EQ(tad->consultar(l, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(tad->remover(l, &v), OK);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(tad->consultar(l, &v), OK);
    ASSERT_EQ(v, 1);

    CASO("limpar zera e a lista continua servindo");
    tad->limpar(l);
    ASSERT_EQ(tad->tamanho(l), 0);
    ASSERT_EQ(tad->inserir_em(l, 0, 1), OK);
    ASSERT_EQ(tad->tamanho(l), 1);

    CASO("lista não tem capacidade");
    ASSERT_EQ(tad->capacidade(l), -1);

    tad->destruir(l);
}

/** Quantos nós continuam acesos ao fim do último trace.
 *
 * Um nó liberado sai da conta: o frontend também o tira dos visitados quando
 * recebe EV_NODE_FREE, senão sobraria um destaque em nó que não existe. */
static int acesos_no_trace(void)
{
    const ev_t *evs = trace_ptr();
    int32_t     n = trace_len();
    int32_t     i, j;
    int32_t     acesos[64];
    int         quantos = 0;

    for (i = 0; i < n; i++) {
        int32_t id = evs[i].a;

        if (evs[i].kind == EV_VISIT) {
            if (quantos < (int) (sizeof acesos / sizeof acesos[0])) {
                acesos[quantos++] = id;
            }
        } else if (evs[i].kind == EV_UNVISIT || evs[i].kind == EV_NODE_FREE) {
            for (j = 0; j < quantos; j++) {
                if (acesos[j] == id) {
                    acesos[j] = acesos[--quantos];
                    break;
                }
            }
        }
    }

    return quantos;
}

/* ---- invariantes específicos ------------------------------------------- *
 * Estes olham por dentro, então usam as funções concretas.                  */

void suite_lista(void)
{
    CASO("lista simples");
    bateria(&LISTA_SIMPLES);

    CASO("lista dupla");
    bateria(&LISTA_DUPLA);

    CASO("lista circular");
    bateria(&LISTA_CIRCULAR);

    /* O invariante do plano para a dupla: n->prox->ant == n em todo nó.
     * Verificado por fora, andando nos dois sentidos e conferindo que as duas
     * travessias dão a mesma sequência invertida. */
    CASO("dupla: ir e voltar dá a mesma sequência");
    {
        ListaDupla *l = lista_dupla_criar();
        elem_t      indo[6], voltando[6];
        int         i;

        for (i = 0; i < 6; i++) {
            ASSERT_EQ(lista_dupla_inserir(l, i, (elem_t) (i + 1)), OK);
        }

        /* De frente para trás, lendo por posição crescente. */
        for (i = 0; i < 6; i++) {
            int achou = lista_dupla_buscar(l, (elem_t) (i + 1), &indo[i]);
            ASSERT_EQ(achou, OK);
            ASSERT_EQ(indo[i], i);
        }

        /* De trás para frente: remover sempre o último tem que devolver
         * 6, 5, 4... Se o ponteiro `ant` estiver errado em algum nó, a
         * travessia pela ponta de trás pega o vizinho errado aqui. */
        for (i = 5; i >= 0; i--) {
            ASSERT_EQ(lista_dupla_remover(l, i, &voltando[i]), OK);
            ASSERT_EQ(voltando[i], (elem_t) (i + 1));
        }
        ASSERT_EQ(lista_dupla_tamanho(l), 0);

        lista_dupla_destruir(l);
    }

    CASO("circular: o ciclo se fecha em qualquer tamanho");
    {
        /* Buscar um valor ausente percorre a lista inteira e volta ao começo.
         * Se o ciclo estivesse quebrado, a travessia leria memória liberada —
         * e é o ASAN do CI que pega isso. Se estivesse sem parada, o teste
         * nunca terminaria. */
        ListaCircular *l = lista_circular_criar();
        int            i, pos = -1;
        elem_t         descartado = 0;

        for (i = 0; i < 5; i++) {
            ASSERT_EQ(lista_circular_inserir(l, i, (elem_t) (i + 1)), OK);
            ASSERT_EQ(lista_circular_buscar(l, 999, &pos), ERR_NAO_ENCONTRADO);
            ASSERT_EQ(lista_circular_tamanho(l), i + 1);
        }

        /* E remover pelo meio mantém o ciclo fechado. */
        ASSERT_EQ(lista_circular_remover(l, 2, &descartado), OK);
        ASSERT_EQ(lista_circular_buscar(l, 999, &pos), ERR_NAO_ENCONTRADO);
        ASSERT_EQ(lista_circular_tamanho(l), 4);

        lista_circular_destruir(l);
    }

    /* A caminhada acende cada nó por onde passa. Se ela não apagar depois, o
     * prefixo inteiro da lista fica destacado na tela para sempre — foi o que
     * aconteceu, e só apareceu no navegador. O trace é a fonte da verdade:
     * inserir e remover não podem deixar nó nenhum aceso. */
    CASO("a caminhada apaga o que acendeu");
    {
        const TAD_Linear *tads[] = { &LISTA_SIMPLES, &LISTA_DUPLA,
                                     &LISTA_CIRCULAR };
        size_t k;

        for (k = 0; k < sizeof tads / sizeof tads[0]; k++) {
            void  *l = tads[k]->criar(0);
            elem_t v = 0;
            int    i;

            for (i = 0; i < 8; i++) {
                ASSERT_EQ(tads[k]->inserir_em(l, i, (elem_t) i), OK);
            }

            trace_reset();
            ASSERT_EQ(tads[k]->inserir_em(l, 5, 99), OK);
            ASSERT_EQ(acesos_no_trace(), 0);

            trace_reset();
            ASSERT_EQ(tads[k]->remover_em(l, 5, &v), OK);
            ASSERT_EQ(v, 99);
            ASSERT_EQ(acesos_no_trace(), 0);

            /* A busca é a exceção: o nó encontrado FICA aceso, porque ele é o
             * resultado. Um só, e é o último visitado. */
            trace_reset();
            ASSERT_EQ(tads[k]->buscar(l, 6, &i), OK);
            ASSERT_EQ(acesos_no_trace(), 1);

            trace_reset();
            ASSERT_EQ(tads[k]->buscar(l, 999, &i), ERR_NAO_ENCONTRADO);
            ASSERT_EQ(acesos_no_trace(), 0);

            tads[k]->destruir(l);
        }
    }

    CASO("tamanho bate com a contagem por travessia");
    {
        /* O invariante que o plano pede para listas. A contagem por travessia
         * aqui é a busca por um valor ausente, que visita todo mundo: o
         * contador de comparações tem que dar exatamente o tamanho. */
        const TAD_Linear *tads[] = { &LISTA_SIMPLES, &LISTA_DUPLA,
                                     &LISTA_CIRCULAR };
        size_t k;

        for (k = 0; k < sizeof tads / sizeof tads[0]; k++) {
            void *l = tads[k]->criar(0);
            int   i, pos = -1;

            for (i = 0; i < 7; i++) {
                ASSERT_EQ(tads[k]->inserir_em(l, i, (elem_t) i), OK);
            }

            trace_reset();
            ASSERT_EQ(tads[k]->buscar(l, 999, &pos), ERR_NAO_ENCONTRADO);
            {
                const ev_t *evs = trace_ptr();
                int32_t     n = trace_len();
                int32_t     j, comparacoes = 0;

                for (j = 0; j < n; j++) {
                    if (evs[j].kind == EV_COUNT && evs[j].a == CNT_COMPARACOES) {
                        comparacoes += evs[j].b;
                    }
                }
                ASSERT_EQ(comparacoes, tads[k]->tamanho(l));
            }

            tads[k]->destruir(l);
        }
    }
}
