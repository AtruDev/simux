/* tests/test_arvore_b.c — a árvore B, e o disco que ela existe para poupar.
 *
 * São três promessas, e `arvore_b_valida` verifica as três juntas:
 *
 *   1. todas as folhas na MESMA profundidade;
 *   2. t-1 <= chaves <= 2t-1 em todo nó que não é raiz;
 *   3. as chaves em ordem, e cada subárvore DENTRO da faixa que o pai delimita.
 *
 * A terceira é a que pega o erro sutil, e é por isso que ela existe. Uma
 * divisão que sobe a chave errada deixa a árvore com a forma perfeita — folhas
 * niveladas, ocupação certa — e a busca quebrada, porque a chave que separa
 * dois filhos deixou de separar o que devia. Verificar só a forma passaria.
 *
 * A segunda metade do arquivo é sobre o disco, que é a razão de esta estrutura
 * existir: a mesma busca que a ABB resolve em vinte acessos, ela resolve em
 * três. */

#include <stddef.h>
#include <math.h>

#include "ds/arvore.h"
#include "ds/arvore_b.h"
#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/paginador.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

enum { MAX_B = 4096 };

/* A invariante e o tamanho de uma vez. Os dois sempre juntos: uma fusão que
 * esquece de decrementar deixa a árvore válida e o contador mentindo. */
static void conferir_b(const ArvoreB *a, int esperado)
{
    static elem_t saida[MAX_B];
    int           k = arvore_b_em_ordem(a, saida, MAX_B);
    int           i;

    ASSERT_TRUE(arvore_b_valida(a));
    ASSERT_EQ(arvore_b_tamanho(a), esperado);
    ASSERT_EQ(k, esperado);

    /* O percurso em ordem sai crescente, como em qualquer árvore de busca. */
    for (i = 1; i < k; i++) {
        ASSERT_TRUE(saida[i - 1] < saida[i]);
    }
}

static void suite_paginador(void)
{
    Paginador p;

    CASO("o paginador conta, e a página 0 é reservada");
    paginador_iniciar(&p);
    ASSERT_EQ(paginador_leituras(&p), 0);
    ASSERT_EQ(paginador_escritas(&p), 0);

    /* A primeira página é a 1: a 0 quer dizer "nenhuma", pelo mesmo motivo
     * que o id 0 é NULL no idmap. */
    ASSERT_EQ(paginador_alocar(&p), 1);
    ASSERT_EQ(paginador_alocar(&p), 2);

    paginador_leu(&p);
    paginador_leu(&p);
    paginador_escreveu(&p);
    ASSERT_EQ(paginador_leituras(&p), 2);
    ASSERT_EQ(paginador_escritas(&p), 1);
}

static void suite_forma(void)
{
    ArvoreB *a;
    int      nivel = -1;
    int      i;

    CASO("a árvore B vazia, e o grau fora da faixa");
    ASSERT_TRUE(arvore_b_criar(1) == NULL);
    ASSERT_TRUE(arvore_b_criar(ARVORE_B_T_MAX + 1) == NULL);

    a = arvore_b_criar(2);
    ASSERT_TRUE(a != NULL);
    conferir_b(a, 0);
    ASSERT_EQ(arvore_b_altura(a), 0);
    ASSERT_EQ(arvore_b_grau(a), 2);
    ASSERT_EQ(arvore_b_buscar(a, 1, &nivel), ERR_NAO_ENCONTRADO);
    ASSERT_EQ(arvore_b_remover(a, 1), ERR_VAZIA);

    CASO("a divisão da raiz é o único jeito de a árvore crescer em altura");
    /* Com t = 2, um nó guarda até 3 chaves. A quarta força a divisão da raiz,
     * e a árvore ganha um nível INTEIRO de uma vez. */
    for (i = 1; i <= 3; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
    }
    conferir_b(a, 3);
    ASSERT_EQ(arvore_b_altura(a), 1);

    ASSERT_EQ(arvore_b_inserir(a, 4), OK);
    conferir_b(a, 4);
    ASSERT_EQ(arvore_b_altura(a), 2);

    CASO("repetida não entra");
    ASSERT_EQ(arvore_b_inserir(a, 2), OK);
    conferir_b(a, 4);

    CASO("a sequência crescente NÃO degenera: é a promessa da árvore B");
    for (i = 5; i <= 200; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
        /* Depois de CADA inserção: uma divisão errada pode se corrigir
         * sozinha na inserção seguinte. */
        ASSERT_TRUE(arvore_b_valida(a));
    }
    conferir_b(a, 200);

    /* 200 chaves com t = 2 cabem em poucos níveis. A ABB com a mesma sequência
     * teria altura 200 — é o mesmo contraste da AVL, agora medido em páginas. */
    ASSERT_TRUE(arvore_b_altura(a) <= 8);

    CASO("tudo o que entrou é encontrado, e nada mais");
    for (i = 1; i <= 200; i++) {
        ASSERT_TRUE(arvore_b_contem(a, i));
        ASSERT_EQ(arvore_b_buscar(a, i, &nivel), OK);
        ASSERT_TRUE(nivel >= 0 && nivel < arvore_b_altura(a));
    }
    ASSERT_TRUE(!arvore_b_contem(a, 0));
    ASSERT_TRUE(!arvore_b_contem(a, 201));

    arvore_b_destruir(a);
}

/* Cada grau muda a forma, e todos têm que continuar válidos. */
static void suite_graus(void)
{
    int t;

    for (t = 2; t <= ARVORE_B_T_MAX; t++) {
        ArvoreB  *a = arvore_b_criar(t);
        Aleatorio rnd;
        int       i;

        CASO("graus");
        ASSERT_TRUE(a != NULL);
        aleatorio_semear(&rnd, 1000u + (uint32_t) t);
        trace_set_enabled(0);

        for (i = 0; i < 300; i++) {
            ASSERT_EQ(arvore_b_inserir(a, aleatorio_entre(&rnd, 0, 999)), OK);
            ASSERT_TRUE(arvore_b_valida(a));
        }

        /* Quanto maior o grau, mais baixa a árvore. É a razão de ela existir. */
        ASSERT_TRUE(arvore_b_altura(a) >= 1);
        if (t >= 4) {
            ASSERT_TRUE(arvore_b_altura(a) <= 5);
        }

        trace_set_enabled(1);
        arvore_b_destruir(a);
    }
}

/* A remoção tem quatro caminhos, e um fuzz aleatório não diz qual quebrou. */
static void suite_remocao(void)
{
    ArvoreB *a;
    int      i;

    CASO("remoção de folha com folga");
    a = arvore_b_criar(3);   /* 2 a 5 chaves por nó */
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i * 10), OK);
    }
    conferir_b(a, 20);
    ASSERT_EQ(arvore_b_remover(a, 200), OK);
    conferir_b(a, 19);
    ASSERT_TRUE(!arvore_b_contem(a, 200));
    arvore_b_destruir(a);

    CASO("remoção da raiz, que é chave interna");
    a = arvore_b_criar(2);
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
    }
    {
        /* A primeira chave da raiz é interna por construção: removê-la exige
         * que alguém — antecessor ou sucessor — suba para o lugar. */
        elem_t todas[MAX_B];
        int    k = arvore_b_em_ordem(a, todas, MAX_B);

        ASSERT_EQ(k, 20);
        ASSERT_EQ(arvore_b_remover(a, todas[10]), OK);
        conferir_b(a, 19);
        ASSERT_TRUE(!arvore_b_contem(a, todas[10]));
    }
    arvore_b_destruir(a);

    CASO("esvaziar em ordem crescente exercita empréstimo e fusão");
    a = arvore_b_criar(2);
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
    }
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_b_remover(a, i), OK);
        ASSERT_TRUE(arvore_b_valida(a));
        ASSERT_EQ(arvore_b_tamanho(a), 60 - i);
    }
    ASSERT_EQ(arvore_b_altura(a), 0);
    conferir_b(a, 0);
    ASSERT_EQ(arvore_b_remover(a, 1), ERR_VAZIA);
    arvore_b_destruir(a);

    CASO("esvaziar em ordem decrescente, que é o caminho espelhado");
    a = arvore_b_criar(2);
    for (i = 1; i <= 60; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
    }
    for (i = 60; i >= 1; i--) {
        ASSERT_EQ(arvore_b_remover(a, i), OK);
        ASSERT_TRUE(arvore_b_valida(a));
    }
    conferir_b(a, 0);
    arvore_b_destruir(a);

    CASO("remover o que não está lá não estraga a árvore");
    a = arvore_b_criar(3);
    for (i = 0; i < 30; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i * 2), OK);
    }
    ASSERT_EQ(arvore_b_remover(a, 7), ERR_NAO_ENCONTRADO);
    conferir_b(a, 30);
    arvore_b_destruir(a);

    CASO("limpar não deixa nó vivo");
    idmap_reset();
    a = arvore_b_criar(2);
    for (i = 0; i < 100; i++) {
        ASSERT_EQ(arvore_b_inserir(a, (i * 37) % 211), OK);
    }
    ASSERT_TRUE(idmap_vivos() > 0);
    arvore_b_limpar(a);
    ASSERT_EQ(idmap_vivos(), 0);
    conferir_b(a, 0);
    ASSERT_EQ(arvore_b_inserir(a, 1), OK);
    conferir_b(a, 1);
    arvore_b_destruir(a);
    arvore_b_destruir(NULL);
    idmap_reset();
}

static void suite_fuzz_b(void)
{
    enum { VALORES = 400, OPERACOES = 8000 };

    ArvoreB  *a = arvore_b_criar(3);
    Aleatorio rnd;
    int       presente[VALORES];
    int       vivos = 0;
    int       i;
    int       divergencias = 0;
    int       removeu = 0;

    CASO("fuzz: a árvore B contra um vetor de presença");
    aleatorio_semear(&rnd, 31415u);
    for (i = 0; i < VALORES; i++) {
        presente[i] = 0;
    }
    trace_set_enabled(0);

    for (i = 0; i < OPERACOES; i++) {
        int    sorteio = aleatorio_entre(&rnd, 0, 99);
        elem_t v = aleatorio_entre(&rnd, 0, VALORES - 1);

        if (sorteio < 55) {
            if (arvore_b_inserir(a, v) != OK) divergencias++;
            if (!presente[v]) {
                presente[v] = 1;
                vivos++;
            }
        } else if (sorteio < 90) {
            int rc = arvore_b_remover(a, v);
            int deveria = presente[v];

            if ((rc == OK) != (deveria != 0)) divergencias++;
            if (rc == OK) {
                removeu++;
                presente[v] = 0;
                vivos--;
            }
        } else {
            int nivel = -1;
            int rc = arvore_b_buscar(a, v, &nivel);

            if ((rc == OK) != (presente[v] != 0)) divergencias++;
        }

        if (arvore_b_tamanho(a) != vivos) divergencias++;
        /* As três promessas depois de CADA operação: é o único jeito de a
         * falha apontar para a operação que a causou. */
        if (!arvore_b_valida(a)) divergencias++;
    }

    for (i = 0; i < VALORES; i++) {
        if (arvore_b_contem(a, i) != (presente[i] != 0)) divergencias++;
    }

    ASSERT_EQ(divergencias, 0);
    ASSERT_TRUE(removeu > 800);

    trace_set_enabled(1);
    arvore_b_destruir(a);
}

/* O "pronto quando" da fase, medido: buscar o mesmo conjunto numa ABB e numa
 * árvore B, e comparar acessos a disco.
 *
 * Numa árvore em memória, cada nó visitado seria uma página lida se ela
 * estivesse em disco — é a mesma equivalência que a árvore B faz explícita. A
 * profundidade da busca, então, É o número de acessos. */
static void suite_o_argumento_do_disco(void)
{
    enum { N = 1000 };

    Abb     *abb = abb_criar();
    ArvoreB *b = arvore_b_criar(8);
    int      i;
    long     antes;
    long     leituras_b;
    int      prof_abb = -1;
    int      nivel_b = -1;

    CASO("disco: a mesma busca, em níveis e em páginas");
    trace_set_enabled(0);

    /* Sequência crescente: o pior caso da ABB e um caso qualquer da B. */
    for (i = 1; i <= N; i++) {
        ASSERT_EQ(abb_inserir(abb, i), OK);
        ASSERT_EQ(arvore_b_inserir(b, i), OK);
    }

    ASSERT_TRUE(arvore_b_valida(b));
    ASSERT_EQ(abb_altura(abb), N);          /* degenerada em lista */
    ASSERT_TRUE(arvore_b_altura(b) <= 4);   /* larga e baixa */

    antes = arvore_b_leituras(b);
    ASSERT_EQ(arvore_b_buscar(b, N, &nivel_b), OK);
    leituras_b = arvore_b_leituras(b) - antes;

    ASSERT_EQ(abb_buscar(abb, N, &prof_abb), OK);

    /* Na ABB, buscar o último elemento passa por mil nós. Na árvore B, por
     * quatro páginas. É a diferença que a fase existe para mostrar, e ela é
     * de duas ordens de grandeza. */
    ASSERT_EQ(prof_abb, N - 1);
    ASSERT_TRUE(leituras_b <= 4);
    ASSERT_TRUE((long) prof_abb > 100 * leituras_b);

    /* A cota teórica: altura <= log_t((n+1)/2) + 1. Com t = 8 e n = 1000,
     * são pouco mais de três níveis. */
    ASSERT_TRUE(
        (double) arvore_b_altura(b)
        <= log((N + 1) / 2.0) / log(8.0) + 1.0 + 1.0);

    trace_set_enabled(1);
    abb_destruir(abb);
    arvore_b_destruir(b);
}

/* Os acessos a disco chegam ao frontend pelo trace, e é de lá que o painel os
 * lê. Se o evento não sair, o número da tela fica em zero para sempre. */
static void suite_eventos_de_disco(void)
{
    ArvoreB *a = arvore_b_criar(2);
    int      i;
    int      leituras = 0;
    int      escritas = 0;
    int      nivel = -1;

    CASO("o trace carrega os acessos a disco");
    for (i = 1; i <= 20; i++) {
        ASSERT_EQ(arvore_b_inserir(a, i), OK);
    }

    trace_reset();
    ASSERT_EQ(arvore_b_buscar(a, 15, &nivel), OK);
    {
        const ev_t *evs = trace_ptr();
        int32_t     n = trace_len();
        int32_t     j;

        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_DISK_READ) leituras++;
            if (evs[j].kind == EV_DISK_WRITE) escritas++;
        }
    }

    /* Uma leitura por nível descido, e nenhuma escrita: buscar não muda nada. */
    ASSERT_EQ(leituras, nivel + 1);
    ASSERT_EQ(escritas, 0);

    /* E o contador do C bate com o que o trace anunciou. */
    {
        long antes = arvore_b_leituras(a);

        ASSERT_EQ(arvore_b_buscar(a, 15, &nivel), OK);
        ASSERT_EQ(arvore_b_leituras(a) - antes, nivel + 1);
    }

    /* Inserir escreve: a página modificada volta para o disco. */
    trace_reset();
    ASSERT_EQ(arvore_b_inserir(a, 999), OK);
    {
        const ev_t *evs = trace_ptr();
        int32_t     n = trace_len();
        int32_t     j;
        int         escreveu = 0;

        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_DISK_WRITE) escreveu++;
        }
        ASSERT_TRUE(escreveu > 0);
    }

    arvore_b_destruir(a);
}

static void suite_vtable_b(void)
{
    CASO("o vtable da árvore B");
    ASSERT_TRUE(ARVORE_B.buscar != NULL);
    ASSERT_TRUE(ARVORE_B.remover_valor != NULL);
    /* Sem "o primeiro" nem "o menor" pela mesma razão do hash: a operação sem
     * argumento não tem sentido definido aqui. */
    ASSERT_TRUE(ARVORE_B.remover == NULL);
    ASSERT_TRUE(ARVORE_B.consultar == NULL);
    ASSERT_TRUE(ARVORE_B.inserir_em == NULL);

    /* Percorrer, sim, e só em ordem — mas não pela mesma razão da ABB. Lá o
     * percurso é a recursão mudando de sentido; aqui ele existe para ser
     * COMPARADO com a varredura da árvore B+, e o que se compara é o preço em
     * páginas. Sozinho ele não ensinaria nada. */
    ASSERT_TRUE(ARVORE_B.percurso != NULL);
    {
        void *a = ARVORE_B.criar(3);

        ASSERT_TRUE(a != NULL);
        ASSERT_EQ(ARVORE_B.percurso(a, PERC_EM_ORDEM), ERR_VAZIA);
        ASSERT_EQ(ARVORE_B.inserir(a, 5), OK);
        ASSERT_EQ(ARVORE_B.percurso(a, PERC_EM_ORDEM), OK);
        ASSERT_EQ(ARVORE_B.percurso(a, PERC_PRE_ORDEM), ERR_ARG_INVALIDO);
        ARVORE_B.destruir(a);
    }
}

void suite_arvore_b(void)
{
    suite_paginador();
    suite_vtable_b();
    suite_forma();
    suite_graus();
    suite_remocao();
    suite_eventos_de_disco();
    suite_fuzz_b();
    suite_o_argumento_do_disco();
}
