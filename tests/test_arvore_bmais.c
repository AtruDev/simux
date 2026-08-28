/* tests/test_arvore_bmais.c — a árvore B+, e a varredura que ela existe para
 * fazer.
 *
 * A árvore B já tinha três promessas, e `arvore_bmais_valida` verifica as
 * mesmas três mais duas que são só daqui:
 *
 *   4. o dado mora só nas folhas — o tamanho é a soma das chaves das folhas, e
 *      as dos nós internos não entram na conta;
 *   5. a corrente de folhas passa por todas elas, em ordem crescente, e
 *      devolve exatamente as chaves que a descida encontra.
 *
 * A quinta é a que pega o erro que só esta estrutura tem. Uma divisão que
 * esquece de remendar o elo, ou uma fusão que o deixa apontando para a página
 * que acabou de morrer, produzem uma árvore que passa em TODA busca — a busca
 * desce, e a descida está certa — e que perde metade das chaves na varredura.
 * Sem a invariante 5, esse bug sai do teste calado.
 *
 * A última suíte é o "pronto quando" da fase: a mesma leitura em ordem, na
 * árvore B e na B+, medida em páginas. */

#include "ds/arvore_b.h"
#include "ds/arvore_bmais.h"
#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

enum { MAX_BM = 4096 };

/* A invariante, o tamanho e a corrente de uma vez.
 *
 * `em_ordem` lê pelo ELO, e não descendo: é a leitura que a estrutura promete,
 * e conferi-la contra o tamanho é o que denuncia um elo perdido. */
static void conferir_bm(const ArvoreBMais *a, int esperado)
{
    static elem_t saida[MAX_BM];
    int           k = arvore_bmais_em_ordem(a, saida, MAX_BM);
    int           i;

    ASSERT_TRUE(arvore_bmais_valida(a));
    ASSERT_EQ(arvore_bmais_tamanho(a), esperado);
    ASSERT_EQ(k, esperado);

    for (i = 1; i < k; i++) {
        ASSERT_TRUE(saida[i - 1] < saida[i]);
    }
}

static void suite_forma_bm(void)
{
    ArvoreBMais *a;
    int          nivel = -1;
    int          i;

    CASO("a árvore B+ vazia, e o grau fora da faixa");
    ASSERT_TRUE(arvore_bmais_criar(1) == NULL);
    ASSERT_TRUE(arvore_bmais_criar(ARVORE_BMAIS_T_MAX + 1) == NULL);

    a = arvore_bmais_criar(2);
    ASSERT_TRUE(a != NULL);
    conferir_bm(a, 0);
    ASSERT_EQ(arvore_bmais_altura(a), 0);
    ASSERT_EQ(arvore_bmais_folhas(a), 0);
    ASSERT_EQ(arvore_bmais_grau(a), 2);
    ASSERT_EQ(arvore_bmais_buscar(a, 1, &nivel), ERR_NAO_ENCONTRADO);
    ASSERT_EQ(arvore_bmais_remover(a, 1), ERR_VAZIA);
    ASSERT_EQ(arvore_bmais_varrer(a), ERR_VAZIA);

    CASO("a raiz é uma folha até a primeira divisão");
    for (i = 1; i <= 3; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }
    conferir_bm(a, 3);
    ASSERT_EQ(arvore_bmais_altura(a), 1);
    ASSERT_EQ(arvore_bmais_folhas(a), 1);

    /* A quarta chave divide a raiz. Numa árvore B a chave do meio subiria e
     * sumiria de baixo; aqui ela é COPIADA, então as quatro continuam nas
     * folhas — e é o total que prova isso. */
    ASSERT_EQ(arvore_bmais_inserir(a, 4), OK);
    conferir_bm(a, 4);
    ASSERT_EQ(arvore_bmais_altura(a), 2);
    ASSERT_EQ(arvore_bmais_folhas(a), 2);

    CASO("repetida não entra");
    ASSERT_EQ(arvore_bmais_inserir(a, 2), OK);
    conferir_bm(a, 4);

    CASO("a sequência crescente não degenera, e as folhas ficam niveladas");
    for (i = 5; i <= 200; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
        ASSERT_TRUE(arvore_bmais_valida(a));
    }
    conferir_bm(a, 200);
    ASSERT_TRUE(arvore_bmais_altura(a) <= 8);

    CASO("a busca desce SEMPRE até a folha");
    /* É o preço da B+, e é exato: não existe achar cedo, porque no meio do
     * caminho não há dado, há roteiro. Toda busca custa a altura. */
    for (i = 1; i <= 200; i++) {
        ASSERT_TRUE(arvore_bmais_contem(a, i));
        ASSERT_EQ(arvore_bmais_buscar(a, i, &nivel), OK);
        ASSERT_EQ(nivel, arvore_bmais_altura(a) - 1);
    }
    ASSERT_TRUE(!arvore_bmais_contem(a, 0));
    ASSERT_TRUE(!arvore_bmais_contem(a, 201));

    CASO("a varredura custa uma página por folha, e nada mais");
    {
        long antes = arvore_bmais_leituras(a);

        ASSERT_EQ(arvore_bmais_varrer(a), OK);
        ASSERT_EQ(arvore_bmais_leituras(a) - antes, arvore_bmais_folhas(a));
    }

    arvore_bmais_destruir(a);
}

static void suite_graus_bm(void)
{
    int t;

    for (t = 2; t <= ARVORE_BMAIS_T_MAX; t++) {
        ArvoreBMais *a = arvore_bmais_criar(t);
        Aleatorio    rnd;
        int          i;

        CASO("graus");
        ASSERT_TRUE(a != NULL);
        aleatorio_semear(&rnd, 2000u + (uint32_t) t);
        trace_set_enabled(0);

        for (i = 0; i < 300; i++) {
            ASSERT_EQ(arvore_bmais_inserir(a, aleatorio_entre(&rnd, 0, 999)), OK);
            ASSERT_TRUE(arvore_bmais_valida(a));
        }

        ASSERT_TRUE(arvore_bmais_altura(a) >= 1);
        if (t >= 4) {
            ASSERT_TRUE(arvore_bmais_altura(a) <= 5);
        }

        trace_set_enabled(1);
        arvore_bmais_destruir(a);
    }
}

static void suite_remocao_bm(void)
{
    ArvoreBMais *a;
    int          i;
    int          nivel = -1;

    CASO("remover da folha com folga");
    a = arvore_bmais_criar(3);
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i * 10), OK);
    }
    conferir_bm(a, 20);
    ASSERT_EQ(arvore_bmais_remover(a, 200), OK);
    conferir_bm(a, 19);
    ASSERT_TRUE(!arvore_bmais_contem(a, 200));
    arvore_bmais_destruir(a);

    CASO("o separador sobrevive à chave que ele nomeia");
    /* Numa B+ a chave que sobe é uma CÓPIA. Removida a de baixo, a de cima
     * continua lá — e continua certa, porque ela nunca foi um dado: só diz
     * "daqui para cima, à direita". A busca desce até a folha de qualquer
     * jeito e não acha nada.
     *
     * Com t = 2 e a sequência crescente, as chaves copiadas para cima são
     * conhecidas; remover todas elas e continuar encontrando o resto é o que
     * prova que o roteiro sobreviveu. */
    a = arvore_bmais_criar(2);
    for (i = 1; i <= 40; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }
    for (i = 3; i <= 40; i += 4) {
        ASSERT_EQ(arvore_bmais_remover(a, i), OK);
        ASSERT_TRUE(arvore_bmais_valida(a));
        ASSERT_EQ(arvore_bmais_buscar(a, i, &nivel), ERR_NAO_ENCONTRADO);
    }
    for (i = 1; i <= 40; i++) {
        int deveria = (i % 4) != 3;

        ASSERT_EQ(arvore_bmais_contem(a, i), deveria);
    }
    arvore_bmais_destruir(a);

    CASO("esvaziar em ordem crescente exercita empréstimo e fusão de folhas");
    a = arvore_bmais_criar(2);
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_bmais_remover(a, i), OK);
        ASSERT_TRUE(arvore_bmais_valida(a));
        ASSERT_EQ(arvore_bmais_tamanho(a), 60 - i);
    }
    ASSERT_EQ(arvore_bmais_altura(a), 0);
    ASSERT_EQ(arvore_bmais_folhas(a), 0);
    conferir_bm(a, 0);
    ASSERT_EQ(arvore_bmais_remover(a, 1), ERR_VAZIA);
    arvore_bmais_destruir(a);

    CASO("esvaziar em ordem decrescente, que é o caminho espelhado");
    a = arvore_bmais_criar(2);
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }
    for (i = 60; i >= 1; i--) {
        ASSERT_EQ(arvore_bmais_remover(a, i), OK);
        ASSERT_TRUE(arvore_bmais_valida(a));
    }
    conferir_bm(a, 0);
    arvore_bmais_destruir(a);

    CASO("remover o que não está lá não estraga a árvore nem a corrente");
    a = arvore_bmais_criar(3);
    for (i = 0; i < 30; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i * 2), OK);
    }
    ASSERT_EQ(arvore_bmais_remover(a, 7), ERR_NAO_ENCONTRADO);
    conferir_bm(a, 30);
    arvore_bmais_destruir(a);

    CASO("limpar não deixa nó vivo");
    idmap_reset();
    a = arvore_bmais_criar(2);
    for (i = 0; i < 100; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, (i * 37) % 211), OK);
    }
    ASSERT_TRUE(idmap_vivos() > 0);
    arvore_bmais_limpar(a);
    ASSERT_EQ(idmap_vivos(), 0);
    conferir_bm(a, 0);
    ASSERT_EQ(arvore_bmais_inserir(a, 1), OK);
    conferir_bm(a, 1);
    arvore_bmais_destruir(a);
    arvore_bmais_destruir(NULL);
    idmap_reset();
}

static void suite_fuzz_bm(void)
{
    enum { VALORES = 400, OPERACOES = 8000 };

    ArvoreBMais *a = arvore_bmais_criar(3);
    Aleatorio    rnd;
    int          presente[VALORES];
    int          vivos = 0;
    int          i;
    int          divergencias = 0;
    int          removeu = 0;

    CASO("fuzz: a árvore B+ contra um vetor de presença");
    aleatorio_semear(&rnd, 27182u);
    for (i = 0; i < VALORES; i++) {
        presente[i] = 0;
    }
    trace_set_enabled(0);

    for (i = 0; i < OPERACOES; i++) {
        int    sorteio = aleatorio_entre(&rnd, 0, 99);
        elem_t v = aleatorio_entre(&rnd, 0, VALORES - 1);

        if (sorteio < 55) {
            if (arvore_bmais_inserir(a, v) != OK) divergencias++;
            if (!presente[v]) {
                presente[v] = 1;
                vivos++;
            }
        } else if (sorteio < 90) {
            int rc = arvore_bmais_remover(a, v);
            int deveria = presente[v];

            if ((rc == OK) != (deveria != 0)) divergencias++;
            if (rc == OK) {
                removeu++;
                presente[v] = 0;
                vivos--;
            }
        } else {
            int nivel = -1;
            int rc = arvore_bmais_buscar(a, v, &nivel);

            if ((rc == OK) != (presente[v] != 0)) divergencias++;
            /* Achando ou não, a busca desce a altura inteira. */
            if (arvore_bmais_altura(a) > 0
                && nivel != arvore_bmais_altura(a) - 1) {
                divergencias++;
            }
        }

        if (arvore_bmais_tamanho(a) != vivos) divergencias++;
        if (!arvore_bmais_valida(a)) divergencias++;
    }

    for (i = 0; i < VALORES; i++) {
        if (arvore_bmais_contem(a, i) != (presente[i] != 0)) divergencias++;
    }

    ASSERT_EQ(divergencias, 0);
    ASSERT_TRUE(removeu > 800);

    trace_set_enabled(1);
    arvore_bmais_destruir(a);
}

/* O "pronto quando" desta metade da fase: a mesma leitura em ordem, nas duas
 * estruturas, medida em páginas.
 *
 * A árvore B sobe e desce, e cada volta ao pai relê a página do pai — não há
 * cache, de propósito. A B+ segue os elos: uma página por folha, e nenhum nó
 * interno tocado. É a diferença que faz praticamente todo índice de banco de
 * dados ser B+ e não B. */
static void suite_a_varredura(void)
{
    enum { N = 500, T = 3 };

    ArvoreB     *b = arvore_b_criar(T);
    ArvoreBMais *bm = arvore_bmais_criar(T);
    int          i;
    long         antes;
    long         leituras_b;
    long         leituras_bm;

    CASO("varredura: a árvore B sobe e desce, a B+ segue a corrente");
    trace_set_enabled(0);

    for (i = 1; i <= N; i++) {
        ASSERT_EQ(arvore_b_inserir(b, i), OK);
        ASSERT_EQ(arvore_bmais_inserir(bm, i), OK);
    }
    ASSERT_TRUE(arvore_b_valida(b));
    ASSERT_TRUE(arvore_bmais_valida(bm));

    antes = arvore_b_leituras(b);
    ASSERT_EQ(arvore_b_varrer(b), OK);
    leituras_b = arvore_b_leituras(b) - antes;

    antes = arvore_bmais_leituras(bm);
    ASSERT_EQ(arvore_bmais_varrer(bm), OK);
    leituras_bm = arvore_bmais_leituras(bm) - antes;

    /* Na B+ o número não é aproximado: é exatamente o número de folhas. */
    ASSERT_EQ(leituras_bm, arvore_bmais_folhas(bm));

    /* E é uma fração do da árvore B, que lê os internos e os relê na volta. */
    ASSERT_TRUE(leituras_b > 2 * leituras_bm);

    /* A busca pontual é o outro lado da moeda, e a B+ perde nele: ela desce
     * sempre, e a árvore B pode parar em cima. Nenhuma das duas é "melhor" —
     * elas respondem a perguntas diferentes, e é isso que a tela mostra. */
    {
        int  nivel_b = -1;
        int  nivel_bm = -1;
        long lidas_b;
        long lidas_bm;

        antes = arvore_b_leituras(b);
        ASSERT_EQ(arvore_b_buscar(b, N / 2, &nivel_b), OK);
        lidas_b = arvore_b_leituras(b) - antes;

        antes = arvore_bmais_leituras(bm);
        ASSERT_EQ(arvore_bmais_buscar(bm, N / 2, &nivel_bm), OK);
        lidas_bm = arvore_bmais_leituras(bm) - antes;

        ASSERT_EQ(nivel_bm, arvore_bmais_altura(bm) - 1);
        ASSERT_EQ(lidas_bm, arvore_bmais_altura(bm));
        ASSERT_TRUE(lidas_b <= lidas_bm);
    }

    trace_set_enabled(1);
    arvore_b_destruir(b);
    arvore_bmais_destruir(bm);
}

/* Os acessos a disco chegam ao frontend pelo trace, e o painel os lê de lá.
 * A corrente também: se o elo não sair no trace, ele não existe na tela. */

static int eh_folha(const int32_t *ids, int quantos, int32_t id)
{
    int i;

    for (i = 0; i < quantos; i++) {
        if (ids[i] == id) {
            return 1;
        }
    }
    return 0;
}

static void suite_eventos_bm(void)
{
    ArvoreBMais *a = arvore_bmais_criar(2);
    int          i;
    int          leituras = 0;
    int          escritas = 0;

    CASO("a varredura no trace: uma leitura por folha, nenhuma escrita");
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }

    trace_reset();
    ASSERT_EQ(arvore_bmais_varrer(a), OK);
    {
        const ev_t *evs = trace_ptr();
        int32_t     n = trace_len();
        int32_t     j;

        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_DISK_READ) leituras++;
            if (evs[j].kind == EV_DISK_WRITE) escritas++;
        }
    }
    ASSERT_EQ(leituras, arvore_bmais_folhas(a));
    ASSERT_EQ(escritas, 0);
    arvore_bmais_destruir(a);

    CASO("o elo entre folhas sai no trace, no slot 0 de quem é folha");
    /* Numa folha o slot 0 de EV_EDGE_SET é a folha SEGUINTE — é o ponteiro
     * que sobra na página —, e CAMPO_FOLHA é o que diz ao frontend que aquele
     * slot é um elo e não um filho. Sem os dois, a corrente não aparece na
     * tela, que é a única coisa que esta estrutura tem a mais. */
    idmap_reset();
    a = arvore_bmais_criar(2);
    trace_reset();
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_bmais_inserir(a, i), OK);
    }
    {
        enum { FOLHAS_MAX = 128 };

        const ev_t *evs = trace_ptr();
        int32_t     n = trace_len();
        int32_t     folhas[FOLHAS_MAX];
        int         quantas = 0;
        int         elos = 0;
        int32_t     j;

        ASSERT_TRUE(!trace_truncado());

        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_NODE_SET && evs[j].b == CAMPO_FOLHA
                && evs[j].c == 1 && quantas < FOLHAS_MAX
                && !eh_folha(folhas, quantas, evs[j].a)) {
                folhas[quantas++] = evs[j].a;
            }
        }
        ASSERT_TRUE(quantas > 1);

        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_EDGE_SET && evs[j].b == 0 && evs[j].c != 0
                && eh_folha(folhas, quantas, evs[j].a)
                && eh_folha(folhas, quantas, evs[j].c)) {
                elos++;
            }
        }
        ASSERT_TRUE(elos > 0);
    }
    arvore_bmais_destruir(a);
    idmap_reset();
}

static void suite_vtable_bm(void)
{
    CASO("o vtable da árvore B+");
    ASSERT_TRUE(ARVORE_B_MAIS.buscar != NULL);
    ASSERT_TRUE(ARVORE_B_MAIS.remover_valor != NULL);
    ASSERT_TRUE(ARVORE_B_MAIS.remover == NULL);
    ASSERT_TRUE(ARVORE_B_MAIS.consultar == NULL);
    ASSERT_TRUE(ARVORE_B_MAIS.inserir_em == NULL);

    /* Percorrer, as duas têm — e só em ordem. Pré e pós-ordem existem numa
     * árvore em que os nós internos guardam dado; aqui eles guardam roteiro. */
    ASSERT_TRUE(ARVORE_B_MAIS.percurso != NULL);
    ASSERT_TRUE(ARVORE_B.percurso != NULL);
    {
        void *a = ARVORE_B_MAIS.criar(3);

        ASSERT_TRUE(a != NULL);
        ASSERT_EQ(ARVORE_B_MAIS.inserir(a, 5), OK);
        ASSERT_EQ(ARVORE_B_MAIS.percurso(a, PERC_EM_ORDEM), OK);
        ASSERT_EQ(ARVORE_B_MAIS.percurso(a, PERC_PRE_ORDEM), ERR_ARG_INVALIDO);
        ASSERT_EQ(ARVORE_B_MAIS.percurso(a, PERC_POS_ORDEM), ERR_ARG_INVALIDO);
        ASSERT_EQ(ARVORE_B_MAIS.capacidade(a), 3);
        ARVORE_B_MAIS.destruir(a);
    }
}

void suite_arvore_bmais(void)
{
    suite_vtable_bm();
    suite_forma_bm();
    suite_graus_bm();
    suite_remocao_bm();
    suite_eventos_bm();
    suite_fuzz_bm();
    suite_a_varredura();
}
