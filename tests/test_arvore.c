/* tests/test_arvore.c — a ABB, e sobretudo a remoção.
 *
 * A invariante é uma só, e é a definição da estrutura: o percurso em ordem sai
 * crescente. Verificar a ordem LOCAL — filho esquerdo menor que o pai — não
 * bastaria, e é o erro clássico de quem escreve este teste: um nó pode ser
 * menor que o pai e maior que o avô, e a árvore estaria quebrada com todos os
 * pares locais certos. O percurso em ordem pega isso.
 *
 * A segunda metade é a remoção. Os três casos precisam ser exercitados de
 * propósito, porque um fuzz aleatório pode passar longe do terceiro — que é
 * justamente o que quebra. Depois disso o fuzz entra, contra um multiconjunto
 * como modelo de referência. */

#include "ds/arvore.h"
#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

enum { MAX = 512 };

/* Verifica a invariante e o tamanho de uma vez.
 *
 * Os dois juntos, sempre: uma remoção que esquece de decrementar `n` deixa a
 * árvore ordenada e o contador mentindo, e o painel de métricas mostraria o
 * número errado sem nada quebrar. */
static void conferir(const Abb *a, int esperado)
{
    elem_t valores[MAX];
    int    k = abb_em_ordem(a, valores, MAX);

    ASSERT_TRUE(abb_ordenada(a));
    ASSERT_EQ(abb_tamanho(a), esperado);
    ASSERT_EQ(k, esperado);
}

/* Insere uma lista de valores, na ordem dada. */
static void inserir_todos(Abb *a, const elem_t *valores, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        ASSERT_EQ(abb_inserir(a, valores[i]), OK);
    }
}

static void suite_forma(void)
{
    Abb *a = abb_criar();

    CASO("a árvore vazia");
    ASSERT_TRUE(a != NULL);
    conferir(a, 0);
    ASSERT_EQ(abb_altura(a), 0);
    {
        elem_t saida = 0;
        int    prof = -1;

        ASSERT_EQ(abb_menor(a, &saida), ERR_VAZIA);
        ASSERT_EQ(abb_remover_menor(a, &saida), ERR_VAZIA);
        ASSERT_EQ(abb_buscar(a, 1, &prof), ERR_NAO_ENCONTRADO);
        ASSERT_EQ(abb_remover(a, 1), ERR_NAO_ENCONTRADO);
    }

    CASO("inserir monta a forma que a ordem de chegada determina");
    {
        /* A árvore-livro: raiz 50, e os dois lados povoados. */
        static const elem_t CENA[] = { 50, 30, 70, 20, 40, 60, 80 };

        inserir_todos(a, CENA, 7);
        conferir(a, 7);
        /* Sete nós numa árvore equilibrada: três níveis. */
        ASSERT_EQ(abb_altura(a), 3);
    }

    CASO("buscar devolve a profundidade, e é ela que mede a árvore");
    {
        int prof = -1;

        ASSERT_EQ(abb_buscar(a, 50, &prof), OK);
        ASSERT_EQ(prof, 0);                     /* a raiz */
        ASSERT_EQ(abb_buscar(a, 30, &prof), OK);
        ASSERT_EQ(prof, 1);
        ASSERT_EQ(abb_buscar(a, 20, &prof), OK);
        ASSERT_EQ(prof, 2);
        ASSERT_EQ(abb_buscar(a, 99, &prof), ERR_NAO_ENCONTRADO);
    }

    CASO("repetido não entra, e não é erro");
    ASSERT_EQ(abb_inserir(a, 50), OK);
    ASSERT_EQ(abb_inserir(a, 20), OK);
    conferir(a, 7);

    CASO("o menor é o nó mais à esquerda");
    {
        elem_t saida = 0;

        ASSERT_EQ(abb_menor(a, &saida), OK);
        ASSERT_EQ(saida, 20);
    }

    abb_destruir(a);
}

/* A sequência crescente é o pior caso da ABB, e é o motivo de a AVL existir.
 * O teste registra o número para a AVL ter contra o que ser comparada. */
static void suite_degenerada(void)
{
    Abb *a = abb_criar();
    int  i;

    CASO("sequência crescente degenera a árvore numa lista");
    for (i = 1; i <= 32; i++) {
        ASSERT_EQ(abb_inserir(a, i), OK);
    }
    conferir(a, 32);

    /* Altura n, e não log n: cada nó novo entrou à direita do anterior. É
     * exatamente o desenho que a aba vai mostrar lado a lado com a AVL. */
    ASSERT_EQ(abb_altura(a), 32);

    {
        int prof = -1;

        /* Buscar o último custa a árvore inteira. */
        ASSERT_EQ(abb_buscar(a, 32, &prof), OK);
        ASSERT_EQ(prof, 31);
    }

    abb_destruir(a);
}

/* Os três casos, um a um, com a forma conferida depois de cada um. */
static void suite_remocao(void)
{
    static const elem_t CENA[] = { 50, 30, 70, 20, 40, 60, 80, 35, 45 };
    Abb *a;

    CASO("remoção, caso 1: folha");
    a = abb_criar();
    inserir_todos(a, CENA, 9);
    conferir(a, 9);
    ASSERT_EQ(abb_remover(a, 20), OK);      /* 20 não tem filho */
    conferir(a, 8);
    {
        int prof = -1;
        ASSERT_EQ(abb_buscar(a, 20, &prof), ERR_NAO_ENCONTRADO);
    }
    abb_destruir(a);

    CASO("remoção, caso 2: um filho só");
    a = abb_criar();
    inserir_todos(a, CENA, 9);
    /* 70 tem 60 e 80; 60 tem só... nada. Usamos 30 depois de podar um lado:
     * removido o 20, o nó 30 fica com um filho só (o 40). */
    ASSERT_EQ(abb_remover(a, 20), OK);
    ASSERT_EQ(abb_remover(a, 30), OK);
    conferir(a, 7);
    {
        int prof = -1;
        /* O filho único subiu, e a subárvore inteira continua alcançável. */
        ASSERT_EQ(abb_buscar(a, 40, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 35, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 45, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 30, &prof), ERR_NAO_ENCONTRADO);
    }
    abb_destruir(a);

    CASO("remoção, caso 3: dois filhos, e o sucessor em ordem sobe");
    a = abb_criar();
    inserir_todos(a, CENA, 9);
    /* 30 tem 20 e 40. O sucessor em ordem de 30 é o 35 — o menor da
     * subárvore direita, alcançado descendo à direita uma vez e à esquerda
     * o resto do caminho. */
    ASSERT_EQ(abb_remover(a, 30), OK);
    conferir(a, 8);
    {
        int prof = -1;

        ASSERT_EQ(abb_buscar(a, 30, &prof), ERR_NAO_ENCONTRADO);
        /* O sucessor tomou o lugar, e não sumiu nem duplicou. */
        ASSERT_EQ(abb_buscar(a, 35, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 20, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 40, &prof), OK);
        ASSERT_EQ(abb_buscar(a, 45, &prof), OK);
    }
    abb_destruir(a);

    CASO("remoção da raiz, que é o caso 3 no lugar mais visível");
    a = abb_criar();
    inserir_todos(a, CENA, 9);
    ASSERT_EQ(abb_remover(a, 50), OK);
    conferir(a, 8);
    {
        elem_t valores[MAX];
        int    k = abb_em_ordem(a, valores, MAX);

        ASSERT_EQ(k, 8);
        /* O sucessor de 50 é o 60, e ele virou a raiz. O percurso continua
         * crescente e sem o 50. */
        ASSERT_EQ(valores[0], 20);
        ASSERT_EQ(valores[7], 80);
    }
    abb_destruir(a);

    CASO("remover o último nó devolve a árvore vazia");
    a = abb_criar();
    ASSERT_EQ(abb_inserir(a, 7), OK);
    ASSERT_EQ(abb_remover(a, 7), OK);
    conferir(a, 0);
    ASSERT_EQ(abb_altura(a), 0);
    ASSERT_EQ(abb_inserir(a, 9), OK);       /* a raiz volta a ser criável */
    conferir(a, 1);
    abb_destruir(a);

    CASO("remover o menor esvazia em ordem crescente");
    a = abb_criar();
    inserir_todos(a, CENA, 9);
    {
        elem_t anterior = -9999;
        int    i;

        for (i = 9; i > 0; i--) {
            elem_t saida = 0;

            ASSERT_EQ(abb_remover_menor(a, &saida), OK);
            ASSERT_TRUE(saida > anterior);
            anterior = saida;
            conferir(a, i - 1);
        }
        ASSERT_EQ(abb_remover_menor(a, &anterior), ERR_VAZIA);
    }
    abb_destruir(a);
}

/* Os percursos, lidos do trace: é o que a tela mostra, e é o que o teste
 * confere. */
static void suite_percursos(void)
{
    static const elem_t CENA[] = { 50, 30, 70, 20, 40, 60, 80 };
    Abb *a = abb_criar();
    int  i;

    /* Quantos EV_VISIT o último percurso emitiu. Cada nó é visitado uma vez,
     * qualquer que seja a ordem — o que muda é QUANDO. */
    int visitas;

    inserir_todos(a, CENA, 7);

    CASO("todo percurso visita cada nó exatamente uma vez");
    for (i = 0; i < PERC_COUNT; i++) {
        const ev_t *evs;
        int32_t     n;
        int32_t     j;

        trace_reset();
        ASSERT_EQ(abb_percurso(a, i), OK);

        evs = trace_ptr();
        n = trace_len();
        visitas = 0;
        for (j = 0; j < n; j++) {
            if (evs[j].kind == EV_VISIT) visitas++;
        }
        ASSERT_EQ(visitas, 7);
    }

    CASO("ordem desconhecida é argumento inválido");
    ASSERT_EQ(abb_percurso(a, PERC_COUNT), ERR_ARG_INVALIDO);
    ASSERT_EQ(abb_percurso(a, -1), ERR_ARG_INVALIDO);

    CASO("o percurso em ordem é o que sai crescente");
    {
        elem_t valores[MAX];
        int    k = abb_em_ordem(a, valores, MAX);

        ASSERT_EQ(k, 7);
        for (i = 1; i < k; i++) {
            ASSERT_TRUE(valores[i - 1] < valores[i]);
        }
        ASSERT_EQ(valores[0], 20);
        ASSERT_EQ(valores[6], 80);
    }

    abb_destruir(a);
}

/* Fuzz contra um multiconjunto trivial.
 *
 * O modelo é um vetor de presença indexado pelo valor. Ele não tem como estar
 * errado, e é contra ele que a árvore é conferida depois de cada operação —
 * é o que pega a remoção que deixa um nó órfão sem quebrar a ordem. */
static void suite_fuzz_arvore(void)
{
    enum { VALORES = 128, OPERACOES = 6000 };

    Abb      *a = abb_criar();
    Aleatorio rnd;
    int       presente[VALORES];
    int       vivos = 0;
    int       i;
    int       divergencias = 0;
    int       removeu = 0;
    int       caso3 = 0;

    CASO("fuzz: a árvore contra um multiconjunto");
    aleatorio_semear(&rnd, 20260827u);
    for (i = 0; i < VALORES; i++) {
        presente[i] = 0;
    }

    /* O trace fica desligado: 6000 operações instrumentadas encheriam o buffer
     * e o teste passaria a medir o truncamento em vez da árvore. */
    trace_set_enabled(0);

    for (i = 0; i < OPERACOES; i++) {
        int    sorteio = aleatorio_entre(&rnd, 0, 99);
        elem_t valor = aleatorio_entre(&rnd, 0, VALORES - 1);

        if (sorteio < 55) {
            if (abb_inserir(a, valor) != OK) divergencias++;
            if (!presente[valor]) {
                presente[valor] = 1;
                vivos++;
            }
        } else if (sorteio < 90) {
            int rc = abb_remover(a, valor);
            int deveria = presente[valor];

            if ((rc == OK) != (deveria != 0)) divergencias++;
            if (rc == OK) {
                removeu++;
                presente[valor] = 0;
                vivos--;
            }
        } else {
            int prof = -1;
            int rc = abb_buscar(a, valor, &prof);

            if ((rc == OK) != (presente[valor] != 0)) divergencias++;
            if (rc == OK && (prof < 0 || prof >= vivos)) divergencias++;
        }

        if (abb_tamanho(a) != vivos) divergencias++;
    }

    /* A ordem é conferida no fim, e não a cada passo: ela é O(n) e 6000 vezes
     * custaria mais que o resto do binário de testes junto. O tamanho, esse, é
     * conferido a cada operação — é barato e pega o mesmo erro cedo. */
    ASSERT_TRUE(abb_ordenada(a));
    ASSERT_EQ(divergencias, 0);
    ASSERT_TRUE(removeu > 500);

    /* Uma varredura final removendo tudo: é aqui que o caso 3 aparece muitas
     * vezes, porque remover sempre a raiz de uma árvore povoada é o caso de
     * dois filhos. */
    for (i = 0; i < VALORES; i++) {
        if (presente[i]) {
            ASSERT_EQ(abb_remover(a, i), OK);
            caso3++;
        }
    }
    ASSERT_TRUE(caso3 > 0);
    ASSERT_EQ(abb_tamanho(a), 0);
    ASSERT_TRUE(abb_ordenada(a));

    trace_set_enabled(1);
    abb_destruir(a);
}

/* Limpar tem que devolver toda a memória: o idmap sabe quantos nós vivos
 * existem, e é ele que responde. */
static void suite_memoria(void)
{
    Abb *a;
    int  i;

    CASO("limpar não deixa nó vivo");
    idmap_reset();
    a = abb_criar();
    for (i = 0; i < 64; i++) {
        ASSERT_EQ(abb_inserir(a, (i * 37) % 101), OK);
    }
    ASSERT_TRUE(idmap_vivos() > 0);

    abb_limpar(a);
    conferir(a, 0);
    ASSERT_EQ(idmap_vivos(), 0);

    /* E dá para usar depois de limpar. */
    ASSERT_EQ(abb_inserir(a, 1), OK);
    conferir(a, 1);

    abb_destruir(a);
    abb_destruir(NULL);
    idmap_reset();
}

/* A árvore entra pelo mesmo vtable das outras, com dois membros opcionais a
 * mais. Quem não é árvore continua com eles nulos. */
static void suite_vtable(void)
{
    CASO("o vtable da árvore");
    ASSERT_TRUE(ABB.remover_valor != NULL);
    ASSERT_TRUE(ABB.percurso != NULL);
    ASSERT_TRUE(ABB.buscar != NULL);
    /* Posição não existe numa árvore. */
    ASSERT_TRUE(ABB.inserir_em == NULL);
    ASSERT_TRUE(ABB.remover_em == NULL);
    /* E quem não é árvore não ganhou os dois novos por acidente. */
    ASSERT_TRUE(PILHA_ENC.remover_valor == NULL);
    ASSERT_TRUE(PILHA_ENC.percurso == NULL);
    ASSERT_TRUE(LISTA_SIMPLES.remover_valor == NULL);
    ASSERT_TRUE(BUSCA_BIN.percurso == NULL);
}

void suite_arvore(void)
{
    suite_vtable();
    suite_forma();
    suite_degenerada();
    suite_remocao();
    suite_percursos();
    suite_memoria();
    suite_fuzz_arvore();
}
