/* tests/test_ordenacao.c — os seis algoritmos contra as seis distribuições.
 *
 * O teste que interessa é uma dupla, e uma metade sozinha não vale nada:
 *
 *   ORDENADO      v[i-1] <= v[i] para todo i
 *   PERMUTAÇÃO    o multiconjunto de saída é o de entrada
 *
 * Um algoritmo que zera o vetor inteiro passa no primeiro com louvor. Um que
 * embaralha sem ordenar passa no segundo. Os dois juntos são a definição de
 * "ordenou", e é assim que cada par algoritmo × distribuição é verificado.
 *
 * A instrumentação é testada junto, e não à parte: o mesmo algoritmo roda com
 * o trace ligado e desligado, e as duas execuções têm que dar o mesmo vetor e
 * as mesmas contagens. Se emitir evento mudasse o resultado, a aba inteira
 * estaria mostrando um algoritmo que não é o que o modo empírico mede. */

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/ordenacao.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "runner.h"

enum { N = 64 };

static const char *NOME_ALG[ALG_COUNT] = {
    "bolha", "selecao", "insercao", "shell", "quick", "merge",
};

/* Roda um algoritmo sobre uma distribuição e verifica a dupla. */
static void ordena_certo(int alg, int dist, int n)
{
    elem_t   antes[N];
    elem_t   depois[N];
    OrdenaFn fn = ordenacao_de(alg);
    int      i;

    ASSERT_TRUE(fn != NULL);
    ASSERT_EQ(cena_gerar(antes, n, dist, 12345u, NULL), OK);

    for (i = 0; i < n; i++) {
        depois[i] = antes[i];
    }

    ASSERT_EQ(fn(depois, n), OK);
    ASSERT_TRUE(cena_ordenado(depois, n));
    ASSERT_TRUE(cena_permutacao(antes, depois, n));
}

/* O vetor e as contagens não podem depender de o trace estar ligado. */
static void trace_nao_muda_nada(int alg)
{
    elem_t   com[N];
    elem_t   sem[N];
    OrdenaFn fn = ordenacao_de(alg);
    long     comparacoes_com;
    long     escritas_com;
    int      i;

    trace_reset();
    trace_set_enabled(1);
    ASSERT_EQ(cena_gerar(com, N, DIST_ALEATORIO, 7u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(fn(com, N), OK);
    comparacoes_com = medida_comparacoes();
    escritas_com = medida_escritas();
    ASSERT_TRUE(trace_len() > 0);

    trace_reset();
    trace_set_enabled(0);
    ASSERT_EQ(cena_gerar(sem, N, DIST_ALEATORIO, 7u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(fn(sem, N), OK);
    ASSERT_EQ(medida_comparacoes(), comparacoes_com);
    ASSERT_EQ(medida_escritas(), escritas_com);
    ASSERT_EQ(trace_len(), 0);

    trace_set_enabled(1);

    for (i = 0; i < N; i++) {
        ASSERT_EQ(sem[i], com[i]);
    }
}

static void suite_todos_ordenam(void)
{
    int alg;
    int dist;

    for (alg = 0; alg < ALG_COUNT; alg++) {
        CASO(NOME_ALG[alg]);

        for (dist = 0; dist < DIST_COUNT; dist++) {
            if (dist == DIST_MANUAL) {
                continue;   /* precisa do vetor de entrada; tem caso próprio */
            }
            ordena_certo(alg, dist, N);
        }

        /* Os tamanhos que quebram laço: um só, o par mínimo, o ímpar mínimo.
         * Um `n - 1` a mais ou a menos em qualquer um deles aparece aqui. */
        ordena_certo(alg, DIST_ALEATORIO, 1);
        ordena_certo(alg, DIST_ALEATORIO, 2);
        ordena_certo(alg, DIST_INVERSO, 2);
        ordena_certo(alg, DIST_ALEATORIO, 3);
    }

    CASO("o trace não muda o resultado");
    for (alg = 0; alg < ALG_COUNT; alg++) {
        trace_nao_muda_nada(alg);
    }
}

/* A cena é o que faz o link compartilhado abrir a mesma tela. Semente igual,
 * vetor igual; semente diferente, vetor diferente. */
static void suite_cena(void)
{
    elem_t a[N];
    elem_t b[N];
    elem_t c[N];
    elem_t manual[4] = { 9, 4, 7, 1 };
    elem_t saida[4];
    int    i;
    int    iguais = 0;

    CASO("cena: a semente reproduz");
    ASSERT_EQ(cena_gerar(a, N, DIST_ALEATORIO, 42u, NULL), OK);
    ASSERT_EQ(cena_gerar(b, N, DIST_ALEATORIO, 42u, NULL), OK);
    ASSERT_EQ(cena_gerar(c, N, DIST_ALEATORIO, 43u, NULL), OK);

    for (i = 0; i < N; i++) {
        ASSERT_EQ(a[i], b[i]);
        if (a[i] == c[i]) iguais++;
    }
    ASSERT_TRUE(iguais < N);

    CASO("cena: cada distribuição é o que promete");
    ASSERT_EQ(cena_gerar(a, N, DIST_ORDENADO, 1u, NULL), OK);
    ASSERT_TRUE(cena_ordenado(a, N));

    ASSERT_EQ(cena_gerar(a, N, DIST_INVERSO, 1u, NULL), OK);
    ASSERT_TRUE(!cena_ordenado(a, N));
    ASSERT_EQ(a[0], N);
    ASSERT_EQ(a[N - 1], 1);

    /* "Poucos distintos" tem que ser poucos de verdade: é o que mata o
     * quicksort de Lomuto, e o teste falha se a geração virar aleatória. */
    ASSERT_EQ(cena_gerar(a, N, DIST_POUCOS_DISTINTOS, 1u, NULL), OK);
    {
        int distintos = 0;
        int j;

        for (i = 0; i < N; i++) {
            int novo = 1;

            for (j = 0; j < i; j++) {
                if (a[j] == a[i]) novo = 0;
            }
            distintos += novo;
        }
        ASSERT_TRUE(distintos <= N / 4);
    }

    /* Quase ordenado é quase: poucas posições fora do lugar. */
    ASSERT_EQ(cena_gerar(a, N, DIST_QUASE_ORDENADO, 1u, NULL), OK);
    {
        int fora = 0;

        for (i = 0; i < N; i++) {
            if (a[i] != i + 1) fora++;
        }
        ASSERT_TRUE(fora > 0);
        ASSERT_TRUE(fora <= N / 4);
    }

    CASO("cena: manual e argumentos inválidos");
    ASSERT_EQ(cena_gerar(saida, 4, DIST_MANUAL, 0u, manual), OK);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(saida[i], manual[i]);
    }
    ASSERT_EQ(cena_gerar(saida, 4, DIST_MANUAL, 0u, NULL), ERR_ARG_INVALIDO);
    ASSERT_EQ(cena_gerar(saida, 0, DIST_ALEATORIO, 0u, NULL), ERR_ARG_INVALIDO);
    ASSERT_EQ(cena_gerar(saida, 4, DIST_COUNT, 0u, NULL), ERR_ARG_INVALIDO);
    ASSERT_EQ(cena_gerar(NULL, 4, DIST_ALEATORIO, 0u, NULL), ERR_ARG_INVALIDO);
}

/* As duas metades do teste de correção, testadas elas mesmas: um verificador
 * que aceita tudo não verifica nada. */
static void suite_verificadores(void)
{
    elem_t antes[4] = { 3, 1, 3, 2 };
    elem_t zerado[4] = { 0, 0, 0, 0 };
    elem_t ordenado[4] = { 1, 2, 3, 3 };
    elem_t sem_repetido[4] = { 1, 2, 3, 4 };

    CASO("verificadores");
    ASSERT_TRUE(cena_ordenado(ordenado, 4));
    ASSERT_TRUE(cena_ordenado(zerado, 4));
    ASSERT_TRUE(!cena_ordenado(antes, 4));

    ASSERT_TRUE(cena_permutacao(antes, ordenado, 4));
    ASSERT_TRUE(!cena_permutacao(antes, zerado, 4));
    /* O 3 repetido é o caso que um verificador ingênuo erra: os dois vetores
     * têm os mesmos valores, em quantidades diferentes. */
    ASSERT_TRUE(!cena_permutacao(antes, sem_repetido, 4));
}

/* As métricas têm que descrever o algoritmo, e não só existir. */
static void suite_metricas(void)
{
    elem_t v[N];
    long   escritas_selecao;
    long   escritas_bolha;

    CASO("métrica: a seleção escreve O(n), a bolha não");
    ASSERT_EQ(cena_gerar(v, N, DIST_INVERSO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(selecao_ordenar(v, N), OK);
    escritas_selecao = medida_escritas();

    ASSERT_EQ(cena_gerar(v, N, DIST_INVERSO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(bolha_ordenar(v, N), OK);
    escritas_bolha = medida_escritas();

    ASSERT_TRUE(escritas_selecao <= 2 * N);
    ASSERT_TRUE(escritas_bolha > escritas_selecao);

    CASO("métrica: a flag da bolha economiza as passadas");
    ASSERT_EQ(cena_gerar(v, N, DIST_ORDENADO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(bolha_ordenar(v, N), OK);
    /* Uma passada e sai: n-1 comparações, não n²/2. */
    ASSERT_EQ(medida_comparacoes(), N - 1);
    ASSERT_EQ(medida_escritas(), 0);

    CASO("métrica: a inserção quase não trabalha em vetor ordenado");
    ASSERT_EQ(cena_gerar(v, N, DIST_ORDENADO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(insercao_ordenar(v, N), OK);
    ASSERT_EQ(medida_comparacoes(), N - 1);

    CASO("métrica: o mergesort não depende da distribuição");
    {
        static const int DISTS[3] = {
            DIST_ALEATORIO, DIST_ORDENADO, DIST_INVERSO
        };
        long por_dist[3];
        int  k;

        for (k = 0; k < 3; k++) {
            ASSERT_EQ(cena_gerar(v, N, DISTS[k], 1u, NULL), OK);
            medida_zerar();
            ASSERT_EQ(merge_ordenar(v, N), OK);
            por_dist[k] = medida_escritas();
        }
        /* Escreve tudo duas vezes por nível, aconteça o que acontecer: as
         * três distribuições dão exatamente o mesmo número. */
        ASSERT_EQ(por_dist[0], por_dist[1]);
        ASSERT_EQ(por_dist[1], por_dist[2]);
    }
}

/* O quicksort com pivô no fim tem pior caso quadrático — e é para isso que
 * ele está aqui. O que NÃO pode acontecer é a recursão acompanhar: com o lado
 * menor na chamada e o maior na iteração, vetor ordenado é O(n²) de tempo e
 * O(1) de pilha. Sem isso, o modo empírico estoura a pilha do wasm. */
static void suite_quick_pior_caso(void)
{
    enum { GRANDE = 4000 };
    static elem_t v[GRANDE];
    long comparacoes_ordenado;
    long comparacoes_aleatorio;

    CASO("quicksort: pior caso sem estourar a pilha");
    ASSERT_EQ(cena_gerar(v, GRANDE, DIST_ORDENADO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(quick_ordenar(v, GRANDE), OK);
    comparacoes_ordenado = medida_comparacoes();
    ASSERT_TRUE(cena_ordenado(v, GRANDE));

    ASSERT_EQ(cena_gerar(v, GRANDE, DIST_ALEATORIO, 1u, NULL), OK);
    medida_zerar();
    ASSERT_EQ(quick_ordenar(v, GRANDE), OK);
    comparacoes_aleatorio = medida_comparacoes();
    ASSERT_TRUE(cena_ordenado(v, GRANDE));

    /* Quadrático contra n log n: a diferença tem que ser de ordem, não de
     * margem. n²/2 = 8 milhões contra ~50 mil. */
    ASSERT_TRUE(comparacoes_ordenado > 10 * comparacoes_aleatorio);
}

/* A tabela é o que a fronteira usa para despachar; um id fora da faixa não
 * pode virar ponteiro. */
static void suite_tabela(void)
{
    int alg;

    CASO("tabela de algoritmos");
    for (alg = 0; alg < ALG_COUNT; alg++) {
        ASSERT_TRUE(ordenacao_de(alg) != NULL);
    }
    ASSERT_TRUE(ordenacao_de(-1) == NULL);
    ASSERT_TRUE(ordenacao_de(ALG_COUNT) == NULL);
}

void suite_ordenacao(void)
{
    suite_tabela();
    suite_verificadores();
    suite_cena();
    suite_todos_ordenam();
    suite_metricas();
    suite_quick_pior_caso();
}
