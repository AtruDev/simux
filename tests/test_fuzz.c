/* tests/test_fuzz.c — fuzz diferencial entre implementações do mesmo TAD.
 *
 * As duas implementações têm a mesma interface, então uma é o oráculo da
 * outra: roda-se a mesma sequência aleatória nas duas e comparam-se as saídas.
 * Não é preciso escrever modelo de referência nenhum.
 *
 * É o teste que acha bug de remoção que ninguém acharia à mão — sobretudo na
 * fila circular, onde o erro aparece só depois de o índice dar a volta um
 * número específico de vezes.
 *
 * A semente é fixa: uma falha aqui tem que ser reexecutável. */

#include <stddef.h>

#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

enum { OPERACOES = 20000, CAPACIDADE = 64 };

/* Roda a mesma sequência nos dois e compara passo a passo.
 *
 * A capacidade é a mesma nos dois lados, e a encadeada respeita o limite por
 * fora — senão elas divergiriam legitimamente no overflow, que não é bug. */
static void comparar(const TAD_Linear *a, const TAD_Linear *b, uint32_t semente)
{
    Aleatorio rnd;
    void     *ea, *eb;
    int       i;
    int       divergencias = 0;

    aleatorio_semear(&rnd, semente);
    ea = a->criar(CAPACIDADE);
    eb = b->criar(CAPACIDADE);
    ASSERT_TRUE(ea != NULL && eb != NULL);

    for (i = 0; i < OPERACOES; i++) {
        int sorteio = aleatorio_entre(&rnd, 0, 99);

        if (sorteio < 55) {
            elem_t valor = aleatorio_entre(&rnd, -1000, 1000);
            int    cheia = a->tamanho(ea) >= CAPACIDADE;
            int    ra, rb;

            if (cheia) {
                continue;   /* o limite é da implementação com vetor, não bug */
            }
            ra = a->inserir(ea, valor);
            rb = b->inserir(eb, valor);
            if (ra != rb) divergencias++;

        } else if (sorteio < 90) {
            elem_t va = 0, vb = 0;
            int    ra = a->remover(ea, &va);
            int    rb = b->remover(eb, &vb);

            if (ra != rb) divergencias++;
            /* O valor removido é o que de fato distingue pilha de fila, e é
             * onde um erro de índice na circular apareceria. */
            if (ra == OK && va != vb) divergencias++;

        } else if (sorteio < 97) {
            elem_t va = 0, vb = 0;
            int    ra = a->consultar(ea, &va);
            int    rb = b->consultar(eb, &vb);

            if (ra != rb) divergencias++;
            if (ra == OK && va != vb) divergencias++;

        } else {
            a->limpar(ea);
            b->limpar(eb);
        }

        if (a->tamanho(ea) != b->tamanho(eb)) divergencias++;
        if (divergencias > 0) {
            break;      /* uma divergência já basta; parar dá relato limpo */
        }
    }

    ASSERT_EQ(divergencias, 0);
    ASSERT_EQ(a->tamanho(ea), b->tamanho(eb));

    a->destruir(ea);
    b->destruir(eb);
}

/* Roda a mesma sequência COM POSIÇÃO nas duas e compara passo a passo.
 *
 * É o teste que as listas precisam e o de cima não dá: o que distingue as três
 * implementações é a caminhada até a posição, e a caminhada só erra em posição
 * que não seja 0 nem n. Sortear a posição dentro da faixa válida a cada
 * operação é o que põe o `andar_ate` de cada uma sob pressão. */
static void comparar_listas(const TAD_Linear *a, const TAD_Linear *b,
                            uint32_t semente)
{
    Aleatorio rnd;
    void     *ea, *eb;
    int       i;
    int       divergencias = 0;

    aleatorio_semear(&rnd, semente);
    ea = a->criar(CAPACIDADE);
    eb = b->criar(CAPACIDADE);
    ASSERT_TRUE(ea != NULL && eb != NULL);
    ASSERT_TRUE(a->inserir_em != NULL && b->inserir_em != NULL);

    for (i = 0; i < OPERACOES; i++) {
        int sorteio = aleatorio_entre(&rnd, 0, 99);
        int n = a->tamanho(ea);

        if (sorteio < 50) {
            elem_t valor = aleatorio_entre(&rnd, -1000, 1000);
            int    pos = aleatorio_entre(&rnd, 0, n);   /* n = no fim */

            if (a->inserir_em(ea, pos, valor) != b->inserir_em(eb, pos, valor)) {
                divergencias++;
            }

        } else if (sorteio < 85) {
            elem_t va = 0, vb = 0;
            /* Sorteia às vezes fora da faixa: recusar igual também é acordo. */
            int    pos = aleatorio_entre(&rnd, -1, n);
            int    ra = a->remover_em(ea, pos, &va);
            int    rb = b->remover_em(eb, pos, &vb);

            if (ra != rb) divergencias++;
            if (ra == OK && va != vb) divergencias++;

        } else if (sorteio < 97) {
            /* Busca por um valor que costuma existir, para o caminho de achar
             * ser exercitado tanto quanto o de não achar. */
            elem_t alvo = aleatorio_entre(&rnd, -20, 20);
            int    pa = -1, pb = -1;
            int    ra = a->buscar(ea, alvo, &pa);
            int    rb = b->buscar(eb, alvo, &pb);

            if (ra != rb) divergencias++;
            if (ra == OK && pa != pb) divergencias++;

        } else {
            a->limpar(ea);
            b->limpar(eb);
        }

        if (a->tamanho(ea) != b->tamanho(eb)) divergencias++;
        if (divergencias > 0) {
            break;
        }
    }

    ASSERT_EQ(divergencias, 0);
    ASSERT_EQ(a->tamanho(ea), b->tamanho(eb));

    a->destruir(ea);
    b->destruir(eb);
}

void suite_fuzz(void)
{
    uint32_t semente;

    /* Sem trace: são vinte mil operações, e o buffer não é o assunto aqui. */
    trace_set_enabled(0);

    for (semente = 1; semente <= 5; semente++) {
        CASO("pilha encadeada e pilha com vetor concordam");
        comparar(&PILHA_ENC, &PILHA_VET, semente);

        CASO("fila encadeada e fila circular concordam");
        comparar(&FILA_ENC, &FILA_VET, semente);
    }

    for (semente = 1; semente <= 5; semente++) {
        CASO("lista simples e lista dupla concordam");
        comparar_listas(&LISTA_SIMPLES, &LISTA_DUPLA, semente);

        CASO("lista simples e lista circular concordam");
        comparar_listas(&LISTA_SIMPLES, &LISTA_CIRCULAR, semente);
    }

    /* Sem posição, a lista age no início — o que é exatamente uma pilha. Não
     * é coincidência, é o contrato que ela assina com o TAD_Linear, e vale
     * verificar que ele se sustenta em vinte mil operações. */
    CASO("lista e pilha concordam nas operações sem posição");
    comparar(&LISTA_SIMPLES, &PILHA_ENC, 3u);

    /* E o contraste que prova que a comparação tem poder de detecção: pilha e
     * fila NÃO podem concordar, senão o fuzz estaria passando à toa. */
    CASO("pilha e fila divergem, como devem");
    {
        Aleatorio rnd;
        void     *p, *f;
        elem_t    vp = 0, vf = 0;
        int       i, diferiu = 0;

        aleatorio_semear(&rnd, 7u);
        p = PILHA_ENC.criar(CAPACIDADE);
        f = FILA_ENC.criar(CAPACIDADE);

        for (i = 0; i < 3; i++) {
            elem_t valor = aleatorio_entre(&rnd, 1, 1000);
            (void) PILHA_ENC.inserir(p, valor);
            (void) FILA_ENC.inserir(f, valor);
        }
        (void) PILHA_ENC.remover(p, &vp);
        (void) FILA_ENC.remover(f, &vf);
        diferiu = (vp != vf);

        ASSERT_TRUE(diferiu);

        PILHA_ENC.destruir(p);
        FILA_ENC.destruir(f);
    }

    trace_set_enabled(1);
    trace_reset();
}
