/* tests/test_busca.c — o vetor ordenado e as duas buscas sobre ele.
 *
 * O teste que vale aqui é o diferencial, e ele sai quase de graça: as duas
 * buscas têm a mesma interface e operam sobre o mesmo vetor, então uma é o
 * oráculo da outra. Um histograma serve de oráculo das duas — sem ele, duas
 * implementações com o mesmo erro concordariam e o teste passaria.
 *
 * O que se compara é o VEREDITO: está lá ou não está. Onde elas param pode
 * divergir legitimamente, porque com valores repetidos a sequencial acha
 * sempre o primeiro e a binária acha um qualquer. Exigir a mesma posição
 * transformaria uma verdade sobre os algoritmos em falha de teste. */

#include <stddef.h>

#include "ds/busca.h"
#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

enum { CAP = 256 };

/* Quantas comparações a última operação emitiu, lidas do trace.
 *
 * O contador do painel é acumulado pelo frontend a partir destes eventos, e é
 * exatamente por isso que o teste lê o trace: assim ele mede o número que o
 * usuário vê, e não um contador paralelo que poderia divergir dele. */
static long comparacoes_no_trace(void)
{
    const ev_t *evs = trace_ptr();
    int32_t     n = trace_len();
    int32_t     i;
    long        total = 0;

    for (i = 0; i < n; i++) {
        if (evs[i].kind == EV_COUNT && evs[i].a == CNT_COMPARACOES) {
            total += evs[i].b;
        }
    }
    return total;
}

static void suite_ordem(void)
{
    VetorOrd *v = vetor_ord_criar(CAP);
    Aleatorio rnd;
    int       i;
    elem_t    saida;

    CASO("o vetor fica ordenado, insira-se na ordem que for");
    ASSERT_TRUE(v != NULL);
    aleatorio_semear(&rnd, 99u);

    for (i = 0; i < 64; i++) {
        ASSERT_EQ(vetor_ord_inserir(v, aleatorio_entre(&rnd, -50, 50)), OK);
        /* Depois de CADA inserção, não só no fim: um deslocamento errado no
         * meio pode se corrigir sozinho na inserção seguinte. */
        ASSERT_TRUE(vetor_ord_ordenado(v));
    }
    ASSERT_EQ(vetor_ord_tamanho(v), 64);

    CASO("remover tira sempre o menor, e a ordem se mantém");
    for (i = 0; i < 64; i++) {
        elem_t antes = 0;
        int    rc_menor = vetor_ord_menor(v, &antes);

        ASSERT_EQ(rc_menor, OK);
        ASSERT_EQ(vetor_ord_remover(v, &saida), OK);
        ASSERT_EQ(saida, antes);
        ASSERT_TRUE(vetor_ord_ordenado(v));
    }
    ASSERT_EQ(vetor_ord_tamanho(v), 0);
    ASSERT_EQ(vetor_ord_remover(v, &saida), ERR_VAZIA);
    ASSERT_EQ(vetor_ord_menor(v, &saida), ERR_VAZIA);

    CASO("a capacidade é fixa, e encher devolve ERR_CHEIA");
    {
        VetorOrd *pequeno = vetor_ord_criar(3);

        ASSERT_EQ(vetor_ord_inserir(pequeno, 1), OK);
        ASSERT_EQ(vetor_ord_inserir(pequeno, 2), OK);
        ASSERT_EQ(vetor_ord_inserir(pequeno, 3), OK);
        ASSERT_EQ(vetor_ord_inserir(pequeno, 4), ERR_CHEIA);
        ASSERT_EQ(vetor_ord_tamanho(pequeno), 3);
        vetor_ord_destruir(pequeno);
    }

    ASSERT_TRUE(vetor_ord_criar(0) == NULL);

    vetor_ord_destruir(v);
    vetor_ord_destruir(NULL);   /* destruir NULL é silêncio, não crash */
}

/* Uma é o oráculo da outra — e um histograma é o oráculo das duas.
 *
 * O modelo de referência é trivial de propósito: um vetor de contagens,
 * indexado pelo valor. Ele não tem como estar errado, e é contra ele que o
 * veredito das duas buscas é conferido. Sem o modelo, duas implementações com
 * o mesmo erro concordariam e o teste passaria. */
static void suite_diferencial(void)
{
    enum { VALORES = 121, FORA = 20 };

    VetorOrd *v = vetor_ord_criar(CAP);
    Aleatorio rnd;
    int       quantos[VALORES];
    int       i;
    int       divergencias = 0;
    int       achadas = 0;
    int       falhadas = 0;

    CASO("fuzz diferencial: sequencial, binária e um histograma");
    aleatorio_semear(&rnd, 4242u);

    for (i = 0; i < VALORES; i++) {
        quantos[i] = 0;
    }
    for (i = 0; i < 200; i++) {
        elem_t valor = aleatorio_entre(&rnd, 0, VALORES - 1);

        ASSERT_EQ(vetor_ord_inserir(v, valor), OK);
        quantos[valor]++;
    }

    /* A faixa procurada é maior que a inserida de propósito: metade das buscas
     * tem que falhar, e o caminho do "não está lá" é onde os dois algoritmos
     * mais diferem. */
    for (i = 0; i < 4000; i++) {
        elem_t alvo = aleatorio_entre(&rnd, -FORA, VALORES - 1 + FORA);
        int    pos_seq = -1;
        int    pos_bin = -1;
        int    rc_seq = vetor_ord_buscar_seq(v, alvo, &pos_seq);
        int    rc_bin = vetor_ord_buscar_bin(v, alvo, &pos_bin);
        int    deveria = (alvo >= 0 && alvo < VALORES && quantos[alvo] > 0);

        if (rc_seq != rc_bin) divergencias++;
        if ((rc_seq == OK) != deveria) divergencias++;
        if ((rc_bin == OK) != deveria) divergencias++;

        if (deveria) {
            achadas++;
            /* Com repetidos elas podem parar em posições diferentes, e isso é
             * verdade sobre os algoritmos, não bug. O que se exige das duas é
             * uma posição dentro do vetor. */
            if (pos_seq < 0 || pos_seq >= vetor_ord_tamanho(v)) divergencias++;
            if (pos_bin < 0 || pos_bin >= vetor_ord_tamanho(v)) divergencias++;
        } else {
            falhadas++;
        }
    }

    ASSERT_EQ(divergencias, 0);
    /* O sorteio tem que ter exercitado os dois caminhos: um fuzz que só acha,
     * ou só falha, testa metade do algoritmo e não avisa. */
    ASSERT_TRUE(achadas > 500);
    ASSERT_TRUE(falhadas > 500);

    vetor_ord_destruir(v);
}

/* O argumento inteiro da aula, medido: O(n) contra O(log n). */
static void suite_custo(void)
{
    VetorOrd *v = vetor_ord_criar(CAP);
    int       i;
    int       pos = -1;
    long      seq;
    long      bin;

    CASO("custo: a binária corta, a sequencial anda");
    for (i = 0; i < 128; i++) {
        ASSERT_EQ(vetor_ord_inserir(v, i * 2), OK);
    }

    /* O último elemento é o pior caso da sequencial e um caso qualquer da
     * binária. É a diferença que o modo comparar mostra na tela. */
    trace_reset();
    ASSERT_EQ(vetor_ord_buscar_seq(v, 254, &pos), OK);
    ASSERT_EQ(pos, 127);
    seq = comparacoes_no_trace();

    trace_reset();
    ASSERT_EQ(vetor_ord_buscar_bin(v, 254, &pos), OK);
    ASSERT_EQ(pos, 127);
    bin = comparacoes_no_trace();

    ASSERT_EQ(seq, 128);
    /* log2(128) = 7, e a binária não pode passar disso mais um. Um número
     * exato aqui travaria o teste numa implementação; a ORDEM é o que importa,
     * e é ela que o teste protege. */
    ASSERT_TRUE(bin <= 8);
    ASSERT_TRUE(seq > 10 * bin);

    CASO("custo: a binária não degrada no valor ausente");
    trace_reset();
    ASSERT_EQ(vetor_ord_buscar_bin(v, 255, &pos), ERR_NAO_ENCONTRADO);
    ASSERT_TRUE(comparacoes_no_trace() <= 8);

    vetor_ord_destruir(v);
}

/* Os limites que quebram busca binária escrita de memória. */
static void suite_bordas(void)
{
    VetorOrd *v = vetor_ord_criar(CAP);
    int       pos = -1;

    CASO("busca no vetor vazio");
    ASSERT_EQ(vetor_ord_buscar_seq(v, 1, &pos), ERR_NAO_ENCONTRADO);
    ASSERT_EQ(vetor_ord_buscar_bin(v, 1, &pos), ERR_NAO_ENCONTRADO);

    CASO("busca no vetor de um elemento");
    ASSERT_EQ(vetor_ord_inserir(v, 7), OK);
    ASSERT_EQ(vetor_ord_buscar_seq(v, 7, &pos), OK);
    ASSERT_EQ(pos, 0);
    ASSERT_EQ(vetor_ord_buscar_bin(v, 7, &pos), OK);
    ASSERT_EQ(pos, 0);
    ASSERT_EQ(vetor_ord_buscar_bin(v, 6, &pos), ERR_NAO_ENCONTRADO);
    ASSERT_EQ(vetor_ord_buscar_bin(v, 8, &pos), ERR_NAO_ENCONTRADO);

    CASO("os extremos, que é onde o lo e o hi erram por um");
    vetor_ord_limpar(v);
    {
        int i;

        for (i = 0; i < 16; i++) {
            ASSERT_EQ(vetor_ord_inserir(v, i), OK);
        }
        /* Primeiro, último, e os dois vizinhos de fora. */
        ASSERT_EQ(vetor_ord_buscar_bin(v, 0, &pos), OK);
        ASSERT_EQ(pos, 0);
        ASSERT_EQ(vetor_ord_buscar_bin(v, 15, &pos), OK);
        ASSERT_EQ(pos, 15);
        ASSERT_EQ(vetor_ord_buscar_bin(v, -1, &pos), ERR_NAO_ENCONTRADO);
        ASSERT_EQ(vetor_ord_buscar_bin(v, 16, &pos), ERR_NAO_ENCONTRADO);

        /* E todos os que estão lá, um por um: um erro de meio some num
         * teste que só olha as pontas. */
        for (i = 0; i < 16; i++) {
            ASSERT_EQ(vetor_ord_buscar_bin(v, i, &pos), OK);
            ASSERT_EQ(pos, i);
        }
    }

    CASO("repetidos: as duas acham, mesmo parando em lugares diferentes");
    vetor_ord_limpar(v);
    {
        int i;
        int pos_seq = -1;
        int pos_bin = -1;

        for (i = 0; i < 8; i++) {
            ASSERT_EQ(vetor_ord_inserir(v, 5), OK);
        }
        ASSERT_EQ(vetor_ord_buscar_seq(v, 5, &pos_seq), OK);
        ASSERT_EQ(vetor_ord_buscar_bin(v, 5, &pos_bin), OK);
        ASSERT_EQ(pos_seq, 0);              /* a sequencial acha o primeiro */
        ASSERT_TRUE(pos_bin >= 0 && pos_bin < 8);
    }

    vetor_ord_destruir(v);
}

/* As duas tabelas do vtable diferem em exatamente um ponteiro, e é isso que a
 * fronteira usa para despachar. */
static void suite_vtable(void)
{
    CASO("as duas tabelas diferem só em buscar");
    ASSERT_TRUE(BUSCA_SEQ.criar == BUSCA_BIN.criar);
    ASSERT_TRUE(BUSCA_SEQ.inserir == BUSCA_BIN.inserir);
    ASSERT_TRUE(BUSCA_SEQ.remover == BUSCA_BIN.remover);
    ASSERT_TRUE(BUSCA_SEQ.consultar == BUSCA_BIN.consultar);
    ASSERT_TRUE(BUSCA_SEQ.buscar != BUSCA_BIN.buscar);

    /* Num vetor ordenado a posição é consequência do valor, não escolha de
     * quem chama: as duas com posição ficam nulas, e api.c responde operação
     * desconhecida a quem pedir. */
    ASSERT_TRUE(BUSCA_SEQ.inserir_em == NULL);
    ASSERT_TRUE(BUSCA_SEQ.remover_em == NULL);
    ASSERT_TRUE(BUSCA_BIN.inserir_em == NULL);
    ASSERT_TRUE(BUSCA_BIN.remover_em == NULL);
}

void suite_busca(void)
{
    suite_vtable();
    suite_ordem();
    suite_bordas();
    suite_diferencial();
    suite_custo();
}
