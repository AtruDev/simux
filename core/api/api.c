/* core/api/api.c — o único arquivo do core que conhece o Emscripten.
 *
 * Todo o resto compila igual nativo e para wasm, o que dá testes rápidos,
 * gdb e um binário de terminal de graça.
 *
 * Desde que existe o vtable TAD_Linear, esta camada não sabe qual estrutura
 * está do outro lado: ela repassa inserir, remover e consultar. Acrescentar a
 * lista dupla não vai mudar uma linha daqui.
 *
 * As sessões são um vetor de slots, e não uma só, porque o modo comparar roda
 * a mesma sequência nas duas implementações do mesmo TAD. ds_call continua
 * sendo (op, a, b, c): o slot não entra na chamada, escolhe-se antes com
 * ds_sessao_slot. Pôr o slot no ds_call custaria o quinto inteiro da fronteira
 * para uma informação que muda uma vez por operação, não por chamada. */

#define TR_SRC SRC_API

#include "ds/api.h"

#include "ds/arvore.h"
#include "ds/arvore_b.h"
#include "ds/busca.h"
#include "ds/hash.h"

#include <stddef.h>
#include <stdlib.h>

#include "ds/erros.h"
#include "ds/fila.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/lista.h"
#include "ds/ordenacao.h"
#include "ds/pilha.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "../ds/linear.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define API EMSCRIPTEN_KEEPALIVE
#else
#define API
#endif

/* Quantas sessões vivem ao mesmo tempo.
 *
 * Era 2, do tempo em que o modo comparar era só pilha e fila — duas
 * implementações do mesmo TAD. Foram ficando famílias maiores: a lista tem
 * três implementações e a família hash tem quatro, e cada trilha em cena
 * precisa da SUA sessão viva o tempo todo, porque as operações chegam uma a
 * uma e todas as trilhas recebem cada uma delas.
 *
 * A aba de ordenação é o caso contrário e por isso não conta: lá o trace
 * inteiro sai de uma vez, e as seis trilhas se revezam num slot só.
 *
 * O custo de um slot vazio é um ponteiro nulo. */
enum { SESSOES = 4 };

/* Teto do vetor da aba de ordenação. O plano pede 5 a 200 no modo animado; a
 * folga existe para o modo corrida e para quem digitar um número maior no
 * campo. Acima disso o trace estoura antes da memória. */
enum { ORD_MAX = 1024 };

/* Teto do modo empírico, que roda com o trace desligado e por isso não tem o
 * buffer de eventos como limite. 65 536 com um algoritmo quadrático são
 * ~4,3 bilhões de comparações — mais que isso e o contador de 32 bits do
 * frontend passaria a mentir. */
enum { BENCH_MAX = 65536 };

typedef struct {
    const TAD_Linear *tad;
    void             *estrutura;
    int32_t           tipo;

    /* A sessão de ordenação não tem TAD: ela é um vetor e um tamanho. É o
     * `tad == NULL` com `tipo == TIPO_ORDENACAO` que a distingue de uma
     * sessão fechada. */
    elem_t           *vetor;
    int32_t           n;
    int32_t           cap;
} Sessao;

static Sessao  g_sessao[SESSOES];
static int32_t g_ativa = 0;
static int32_t g_erro  = OK;

/* Buffer de entrada: o único caminho para um dado que não cabe nos quatro
 * inteiros de ds_call. Hoje serve à distribuição manual da ordenação — o JS
 * escreve os valores aqui e chama OP_GERAR com DIST_MANUAL. */
static elem_t *g_entrada;
static int32_t g_entrada_cap;

/* Resultado do último ds_bench. Duas métricas, e ds_bench só pode devolver
 * um inteiro: a segunda sai por getter em vez de o modo empírico precisar de
 * duas passadas idênticas. */
static int32_t g_bench_escritas;

#define ATIVA (g_sessao[g_ativa])

/* As funções das estruturas devolvem OK ou um ERR_*; a fronteira devolve -1 e
 * guarda o motivo, porque é isso que o JS sabe testar. */
static int32_t concluir(int rc)
{
    if (rc != OK) {
        g_erro = rc;
        return -1;
    }
    return OK;
}

static const TAD_Linear *tad_de(int32_t tipo)
{
    switch (tipo) {
    case TIPO_PILHA_ENC: return &PILHA_ENC;
    case TIPO_PILHA_VET: return &PILHA_VET;
    case TIPO_FILA_ENC:  return &FILA_ENC;
    case TIPO_FILA_VET:  return &FILA_VET;
    case TIPO_LISTA_SIMPLES:  return &LISTA_SIMPLES;
    case TIPO_LISTA_DUPLA:    return &LISTA_DUPLA;
    case TIPO_LISTA_CIRCULAR: return &LISTA_CIRCULAR;
    case TIPO_BUSCA_SEQ: return &BUSCA_SEQ;
    case TIPO_BUSCA_BIN: return &BUSCA_BIN;
    case TIPO_ABB:       return &ABB;
    case TIPO_AVL:       return &AVL;
    case TIPO_HASH_ENC:    return &HASH_ENC;
    case TIPO_HASH_LINEAR: return &HASH_LINEAR;
    case TIPO_HASH_QUAD:   return &HASH_QUAD;
    case TIPO_HASH_DUPLO:  return &HASH_DUPLO;
    case TIPO_ARVORE_B:    return &ARVORE_B;
    default:             return NULL;
    }
}

/* Verdadeiro enquanto qualquer slot ainda tiver estrutura viva. */
static int alguma_aberta(void)
{
    int i;

    for (i = 0; i < SESSOES; i++) {
        if (g_sessao[i].estrutura != NULL) {
            return 1;
        }
    }
    return 0;
}

static int eh_ordenacao(const Sessao *s)
{
    return s->tipo == TIPO_ORDENACAO && s->vetor != NULL;
}

/* Escolhe o slot sobre o qual as próximas chamadas operam. */
API int32_t ds_sessao_slot(int32_t slot)
{
    if (slot < 0 || slot >= SESSOES) {
        g_erro = ERR_ARG_INVALIDO;
        return -1;
    }
    g_ativa = slot;
    return OK;
}

API int32_t ds_sessao_slots(void)
{
    return SESSOES;
}

API void ds_sessao_fim(void)
{
    if (ATIVA.estrutura != NULL && ATIVA.tad != NULL) {
        ATIVA.tad->destruir(ATIVA.estrutura);
    }
    free(ATIVA.vetor);
    ATIVA.vetor = NULL;
    ATIVA.n = 0;
    ATIVA.cap = 0;

    ATIVA.estrutura = NULL;
    ATIVA.tad = NULL;
    ATIVA.tipo = TIPO_NENHUM;

    /* A numeração recomeça do 1 a cada sessão: os ids são identidade visual, e
     * nada da sessão anterior continua na tela.
     *
     * Mas só quando a última fecha. Zerar com o outro slot aberto renumeraria
     * nós que continuam na tela — os ids que o frontend já recebeu deixariam
     * de valer, e a comparação passaria a desenhar sobre o nó errado. */
    if (!alguma_aberta()) {
        idmap_reset();
    }
}

API int32_t ds_sessao_nova(int32_t tipo, int32_t capacidade)
{
    const TAD_Linear *tad = tad_de(tipo);

    ds_sessao_fim();
    trace_reset();
    g_erro = OK;

    /* A sessão de ordenação não passa pelo vtable: ela não tem inserir nem
     * remover, tem um vetor. Reservar aqui, e não a cada OP_GERAR, é o que
     * deixa trocar de distribuição sem realocar. */
    if (tipo == TIPO_ORDENACAO) {
        if (capacidade <= 0 || capacidade > ORD_MAX) {
            return concluir(ERR_ARG_INVALIDO);
        }
        ATIVA.vetor = malloc((size_t) capacidade * sizeof *ATIVA.vetor);
        if (ATIVA.vetor == NULL) {
            return concluir(ERR_SEM_MEMORIA);
        }
        ATIVA.cap = capacidade;
        ATIVA.n = 0;
        ATIVA.tipo = tipo;
        return OK;
    }

    if (tad == NULL) {
        return concluir(ERR_ARG_INVALIDO);
    }

    /* A criação já emite eventos — EV_ARR_INIT e os ponteiros iniciais —,
     * então o trace precisa estar zerado antes dela, e não depois. */
    ATIVA.estrutura = tad->criar(capacidade);
    if (ATIVA.estrutura == NULL) {
        return concluir(ERR_SEM_MEMORIA);
    }

    ATIVA.tad = tad;
    ATIVA.tipo = tipo;
    return OK;
}

API int32_t ds_tipo_sessao(void)
{
    return ATIVA.tipo;
}

/* Tamanho de verdade da estrutura, que não é o do modelo do frontend: aquele
 * reflete onde a animação está, e este reflete tudo o que já foi executado.
 * Quem precisa inserir "no fim" precisa deste. */
API int32_t ds_tamanho(void)
{
    if (eh_ordenacao(&ATIVA)) {
        return ATIVA.n;
    }
    return (ATIVA.tad != NULL) ? ATIVA.tad->tamanho(ATIVA.estrutura) : 0;
}

API int32_t ds_capacidade(void)
{
    if (eh_ordenacao(&ATIVA)) {
        return ATIVA.cap;
    }
    return (ATIVA.tad != NULL) ? ATIVA.tad->capacidade(ATIVA.estrutura) : -1;
}

/* ---- ordenação --------------------------------------------------------- */

/* Preenche o vetor da sessão com uma distribuição. `c` é a semente: a mesma
 * dá o mesmo vetor em qualquer máquina, e é isso que faz um link
 * compartilhado abrir a mesma cena. */
static int gerar(int32_t n, int32_t dist, unsigned int semente)
{
    int rc;

    if (n <= 0 || n > ATIVA.cap) {
        return ERR_ARG_INVALIDO;
    }

    rc = cena_gerar(ATIVA.vetor, n, dist, semente, g_entrada);
    if (rc != OK) {
        return rc;
    }

    ATIVA.n = n;
    return OK;
}

static int ordenar(int32_t alg)
{
    OrdenaFn fn = ordenacao_de(alg);
    int      rc;
    int      i;

    if (fn == NULL) {
        return ERR_ARG_INVALIDO;
    }
    if (ATIVA.n <= 0) {
        return ERR_VAZIA;
    }

    medida_zerar();
    rc = fn(ATIVA.vetor, ATIVA.n);
    if (rc != OK) {
        return rc;
    }

    /* A varredura final é daqui, e não de dentro de cada algoritmo: seis
     * cópias do mesmo laço seriam seis lugares para esquecer. O que os
     * algoritmos marcam durante a execução é outra coisa — é a fronteira
     * crescendo, que faz parte de como cada um funciona. */
    /* A faixa volta a ser o vetor inteiro antes das marcas. Ela é o trecho que
     * o algoritmo estava olhando, e no fim ele não está olhando trecho nenhum
     * — sem isto, a última partição continuava apagando o resto da tela. */
    TR(EV_ARR_RANGE, .a = 0, .b = ATIVA.n - 1);
    for (i = 0; i < ATIVA.n; i++) {
        TR(EV_ARR_MARK, .a = i, .b = TAG_ORDENADO);
    }
    TR(EV_MSG, .a = STR_ORDENADO);

    return OK;
}

API int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c)
{
    elem_t descartado;
    int    posicao = 0;

    (void) c;

    trace_reset();
    g_erro = OK;

    if (op == OP_PING) {
        TR(EV_MSG, .a = STR_PING);
        return OK;
    }

    /* A sessão de ordenação atende duas operações e nenhuma do vtable. Ela é
     * testada antes do `tad == NULL` porque, para ela, tad NULO é o normal. */
    if (ATIVA.tipo == TIPO_ORDENACAO) {
        switch (op) {
        case OP_GERAR:  return concluir(gerar(a, b, (unsigned int) c));
        case OP_ORDENAR: return concluir(ordenar(a));
        default:        return concluir(ERR_OP_DESCONHECIDA);
        }
    }

    if (ATIVA.tad == NULL) {
        return concluir(ERR_SEM_SESSAO);
    }

    switch (op) {
    case OP_PUSH:
        return concluir(ATIVA.tad->inserir(ATIVA.estrutura, a));

    /* Remover e consultar SEM argumento não existem em toda estrutura. Numa
     * tabela hash não há "o primeiro" nem "o menor" — a ordem dos elementos é
     * acidente da função hash, e devolver um deles seria inventar semântica.
     * O ponteiro nulo no vtable é a resposta, como sempre. */
    case OP_POP:
        if (ATIVA.tad->remover == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->remover(ATIVA.estrutura, &descartado));

    case OP_TOPO:
        if (ATIVA.tad->consultar == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->consultar(ATIVA.estrutura, &descartado));

    case OP_LIMPAR:
        if (ATIVA.tad->limpar == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        ATIVA.tad->limpar(ATIVA.estrutura);
        return OK;

    /* As três com posição existem só em quem tem posição. O ponteiro nulo no
     * vtable é a resposta: pedir isso a uma pilha é operação desconhecida, não
     * argumento inválido. */
    case OP_INSERIR_EM:
        if (ATIVA.tad->inserir_em == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->inserir_em(ATIVA.estrutura, b, a));

    case OP_REMOVER_EM:
        if (ATIVA.tad->remover_em == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->remover_em(ATIVA.estrutura, b, &descartado));

    /* Remover por VALOR é da árvore: nela a posição não existe, e é essa
     * remoção que traz os três casos. O ponteiro nulo no vtable responde por
     * quem não tem — pedir isso a uma pilha é operação desconhecida. */
    case OP_REMOVER_VALOR:
        if (ATIVA.tad->remover_valor == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->remover_valor(ATIVA.estrutura, a));

    case OP_PERCURSO:
        if (ATIVA.tad->percurso == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->percurso(ATIVA.estrutura, a));

    case OP_BUSCAR:
        if (ATIVA.tad->buscar == NULL) {
            return concluir(ERR_OP_DESCONHECIDA);
        }
        return concluir(ATIVA.tad->buscar(ATIVA.estrutura, a, &posicao));

    default:
        return concluir(ERR_OP_DESCONHECIDA);
    }
}

API int32_t ds_erro(void)
{
    return g_erro;
}

API const ev_t *ds_trace_ptr(void)
{
    return trace_ptr();
}

API int32_t ds_trace_len(void)
{
    return trace_len();
}

API int32_t ds_trace_truncado(void)
{
    return trace_truncado();
}

/* Buffer de entrada, para o dado que não cabe nos quatro inteiros de ds_call.
 *
 * Devolve um ponteiro para dentro da heap do wasm: o JS monta um Int32Array
 * sobre ele, escreve os valores e chama OP_GERAR com DIST_MANUAL. É a
 * fronteira de dados do projeto, e continua sem string e sem parser.
 *
 * A view do lado do JS tem que ser recriada depois desta chamada como depois
 * de qualquer outra — este malloc é justamente um dos que podem crescer a
 * heap e desanexar as views antigas. */
API elem_t *ds_buffer(int32_t n)
{
    elem_t *maior;

    if (n <= 0 || n > ORD_MAX) {
        g_erro = ERR_ARG_INVALIDO;
        return NULL;
    }
    if (n <= g_entrada_cap) {
        return g_entrada;
    }

    maior = realloc(g_entrada, (size_t) n * sizeof *maior);
    if (maior == NULL) {
        g_erro = ERR_SEM_MEMORIA;
        return NULL;
    }

    g_entrada = maior;
    g_entrada_cap = n;
    return g_entrada;
}

/* Modo empírico: roda um algoritmo com o trace DESLIGADO e devolve quantas
 * comparações ele fez.
 *
 * Desligar o trace é o ponto inteiro. Medir n = 25 600 com ele ligado geraria
 * centenas de milhões de eventos para ninguém assistir — e as macros de
 * core/sort/passos.h continuam contando, porque o contador e o evento saem da
 * mesma linha. É o que permite plotar comparações medidas contra n e comparar
 * com as curvas teóricas.
 *
 * O vetor é próprio, e não o da sessão: o modo empírico roda para n muito
 * maiores que o animado, e não deve mexer no que está na tela. */
API int32_t ds_bench(int32_t alg, int32_t n, int32_t dist, int32_t semente)
{
    OrdenaFn fn = ordenacao_de(alg);
    elem_t  *v;
    int      ligado;
    int      rc;
    int      ordenado;
    long     comparacoes;

    trace_reset();
    g_erro = OK;
    g_bench_escritas = 0;

    if (fn == NULL || n <= 0 || n > BENCH_MAX) {
        return concluir(ERR_ARG_INVALIDO);
    }

    v = malloc((size_t) n * sizeof *v);
    if (v == NULL) {
        return concluir(ERR_SEM_MEMORIA);
    }

    ligado = trace_enabled();
    trace_set_enabled(0);
    medida_zerar();

    rc = cena_gerar(v, n, dist, (unsigned int) semente, g_entrada);
    if (rc == OK) {
        rc = fn(v, n);
    }
    ordenado = cena_ordenado(v, n);

    trace_set_enabled(ligado);
    comparacoes = medida_comparacoes();
    g_bench_escritas = (int32_t) medida_escritas();
    free(v);

    if (rc != OK) {
        return concluir(rc);
    }
    /* Um algoritmo que devolve OK e um vetor fora de ordem é bug, e a medida
     * dele não vale nada. Melhor a curva não aparecer do que aparecer errada. */
    if (!ordenado) {
        return concluir(ERR_ARG_INVALIDO);
    }

    return (int32_t) comparacoes;
}

/* Escritas do último ds_bench. Separado porque ds_bench já usa o valor de
 * retorno para as comparações, e medir duas vezes custaria o dobro. */
API int32_t ds_bench_escritas(void)
{
    return g_bench_escritas;
}
