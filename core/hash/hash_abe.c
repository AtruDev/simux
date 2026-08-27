/* core/hash/hash_abe.c — hash com endereçamento aberto, nas três sondagens.
 *
 * Colidiu, procura outra célula no próprio arranjo. Não aloca nada por
 * elemento, e em troca a tabela enche — e piora muito antes de encher, porque
 * as células ocupadas se juntam e cada colisão passa a custar mais que a
 * anterior. É o agrupamento, e é a coisa que esta tela existe para mostrar.
 *
 * As três implementações diferem em UMA função: por quanto andar na tentativa
 * `k`. É o mesmo arranjo das duas buscas do vetor ordenado, e lado a lado o
 * que muda entre elas é exatamente o desenho do agrupamento.
 *
 *   linear      +1, +2, +3, ...     agrupamento primário: blocos contíguos
 *                                   crescem e se fundem
 *   quadrática  +1, +4, +9, ...     quebra os blocos, mas duas chaves com o
 *                                   mesmo h(k) percorrem a MESMA sequência —
 *                                   é o agrupamento secundário
 *   dupla       +h2, +2h2, +3h2     o passo depende da chave, então nem isso
 *                                   acontece
 *
 * A REMOÇÃO é o outro assunto, e é onde quase toda implementação caseira erra.
 * Apagar a célula quebra a sondagem: quem estava depois dela na sequência
 * deixa de ser encontrado, porque a busca para na primeira célula vazia. A
 * solução é o TÚMULO — a célula fica marcada como "já foi ocupada", a busca
 * atravessa e a inserção pode reaproveitar. O preço é que a tabela piora com o
 * uso mesmo sem crescer, e o contador de túmulos é o que torna isso visível. */

#define TR_SRC SRC_HASH_ABE

#include "ds/hash.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/trace.h"

#include "linear.h"

/* O estado de uma célula. Três, e não dois: é justamente o terceiro que faz a
 * remoção funcionar. */
typedef enum {
    VAZIA,
    OCUPADA,
    TUMULO
} Estado;

struct HashAbe {
    elem_t *chaves;
    Estado *estado;
    int     m;
    int     n;
    int     tumulos;
    int     sondagem;   /* TIPO_HASH_LINEAR | _QUAD | _DUPLO */
};

static int h1(const HashAbe *h, elem_t chave)
{
    int i = (int) (chave % h->m);

    return (i < 0) ? i + h->m : i;
}

/* O segundo hash da sondagem dupla.
 *
 * Tem que devolver algo entre 1 e m-1, e nunca zero: passo zero faria a
 * sondagem ficar parada na mesma célula para sempre. E tem que ser primo com
 * m para a sequência visitar a tabela inteira — com m primo, qualquer passo
 * entre 1 e m-1 serve, e é por isso que a sondagem dupla pede m primo. */
static int h2(const HashAbe *h, elem_t chave)
{
    int i;

    if (h->m <= 2) {
        return 1;
    }
    i = (int) (chave % (h->m - 1));
    if (i < 0) {
        i += h->m - 1;
    }
    return 1 + i;
}

/* Por quanto andar na tentativa k. É a única coisa que separa as três. */
static int passo(const HashAbe *h, elem_t chave, int k)
{
    switch (h->sondagem) {
    case TIPO_HASH_QUAD:  return k * k;
    case TIPO_HASH_DUPLO: return k * h2(h, chave);
    default:              return k;      /* linear */
    }
}

static int celula(const HashAbe *h, elem_t chave, int k)
{
    /* O módulo depois da soma, e a soma em long: com m grande e k grande,
     * k*k estoura o int antes de o módulo ter chance de agir. */
    long onde = (long) h1(h, chave) + (long) passo(h, chave, k);

    return (int) (((onde % h->m) + h->m) % h->m);
}

HashAbe *hash_abe_criar(int m, int sondagem)
{
    HashAbe *h;
    int      i;

    if (m <= 0) {
        return NULL;
    }

    h = malloc(sizeof *h);
    if (h == NULL) {
        return NULL;
    }

    h->chaves = malloc((size_t) m * sizeof *h->chaves);
    h->estado = malloc((size_t) m * sizeof *h->estado);
    if (h->chaves == NULL || h->estado == NULL) {
        free(h->chaves);
        free(h->estado);
        free(h);
        return NULL;
    }

    h->m = m;
    h->n = 0;
    h->tumulos = 0;
    h->sondagem = sondagem;

    for (i = 0; i < m; i++) {
        h->chaves[i] = 0;
        h->estado[i] = VAZIA;
    }

    TR(EV_ARR_INIT, .a = m);
    return h;
}

int hash_abe_inserir(HashAbe *h, elem_t valor)
{
    int primeiro_tumulo = -1;
    int k;

    TR(EV_PTR_SET, .a = PTR_BALDE, .b = h1(h, valor));
    TR(EV_MSG, .a = STR_BALDE);

    /* A sondagem anda no máximo m vezes: mais que isso e ela estaria repetindo
     * células. É esse limite que faz a tabela cheia devolver erro em vez de
     * girar para sempre. */
    for (k = 0; k < h->m; k++) {
        int i = celula(h, valor, k);

        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = i);
        TR(EV_COUNT, .a = CNT_SONDAGENS, .b = +1);

        if (h->estado[i] == VAZIA) {
            /* Célula nunca usada: a chave não está na tabela, e este é o lugar
             * dela — a não ser que um túmulo mais cedo na sequência sirva. */
            int destino = (primeiro_tumulo >= 0) ? primeiro_tumulo : i;

            if (destino != i) {
                h->tumulos--;
            }
            h->chaves[destino] = valor;
            h->estado[destino] = OCUPADA;

            TR(EV_ARR_WRITE, .a = destino, .b = valor);
            TR(EV_ARR_MARK, .a = destino, .b = TAG_NENHUMA);
            TR(EV_COUNT, .a = CNT_ESCRITAS, .b = +1);

            h->n++;
            TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
            return OK;
        }

        if (h->estado[i] == TUMULO) {
            /* Guarda o primeiro túmulo e CONTINUA: parar aqui poderia inserir
             * uma segunda cópia de uma chave que está mais adiante. */
            TR(EV_MSG, .a = STR_TUMULO);
            if (primeiro_tumulo < 0) {
                primeiro_tumulo = i;
            }
            continue;
        }

        TR(EV_ARR_COMPARE, .a = i, .b = 0, .c = 1);
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (h->chaves[i] == valor) {
            TR(EV_MSG, .a = STR_JA_EXISTE);
            return OK;
        }

        /* Ocupada por outra chave: colisão, e a sondagem continua. */
        TR(EV_MSG, .a = (k == 0) ? STR_COLISAO : STR_SONDANDO);
        if (k == 0) {
            TR(EV_COUNT, .a = CNT_COLISOES, .b = +1);
        }
    }

    /* Deu a volta sem achar lugar. Com sondagem quadrática isso pode acontecer
     * com a tabela ainda meio vazia — a sequência de saltos não visita todas
     * as células —, e é uma das coisas que a quadrática cobra. */
    if (primeiro_tumulo >= 0) {
        h->chaves[primeiro_tumulo] = valor;
        h->estado[primeiro_tumulo] = OCUPADA;
        h->tumulos--;
        TR(EV_ARR_WRITE, .a = primeiro_tumulo, .b = valor);
        TR(EV_ARR_MARK, .a = primeiro_tumulo, .b = TAG_NENHUMA);
        h->n++;
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = +1);
        return OK;
    }

    TR(EV_MSG, .a = STR_TABELA_CHEIA);
    return ERR_CHEIA;
}

int hash_abe_buscar(const HashAbe *h, elem_t valor, int *posicao)
{
    int k;

    TR(EV_PTR_SET, .a = PTR_BALDE, .b = h1(h, valor));
    TR(EV_MSG, .a = STR_BALDE);

    for (k = 0; k < h->m; k++) {
        int i = celula(h, valor, k);

        TR(EV_PTR_SET, .a = PTR_CURSOR, .b = i);
        TR(EV_COUNT, .a = CNT_SONDAGENS, .b = +1);

        /* Célula nunca usada encerra a busca: se a chave estivesse na tabela,
         * ela teria parado aqui na inserção. O túmulo NÃO encerra — é
         * exatamente para isso que ele existe. */
        if (h->estado[i] == VAZIA) {
            break;
        }

        if (h->estado[i] == TUMULO) {
            TR(EV_MSG, .a = STR_TUMULO);
            continue;
        }

        TR(EV_ARR_COMPARE, .a = i, .b = 0, .c = 1);
        TR(EV_COUNT, .a = CNT_COMPARACOES, .b = +1);

        if (h->chaves[i] == valor) {
            TR(EV_ARR_MARK, .a = i, .b = TAG_PIVO);
            TR(EV_MSG, .a = STR_ACHOU);
            *posicao = i;
            return OK;
        }

        TR(EV_MSG, .a = STR_SONDANDO);
    }

    TR(EV_MSG, .a = STR_NAO_ACHOU);
    return ERR_NAO_ENCONTRADO;
}

int hash_abe_remover(HashAbe *h, elem_t valor)
{
    int posicao = -1;
    int rc = hash_abe_buscar(h, valor, &posicao);

    if (rc != OK) {
        return rc;
    }

    /* Túmulo, e não célula vazia. Apagar quebraria a sondagem de quem está
     * depois: a busca pararia aqui e o elemento seguinte sumiria da tabela
     * sem nunca ter sido removido. */
    h->estado[posicao] = TUMULO;
    h->tumulos++;
    TR(EV_ARR_MARK, .a = posicao, .b = TAG_LIVRE);
    TR(EV_MSG, .a = STR_TUMULO);

    h->n--;
    TR(EV_COUNT, .a = CNT_TAMANHO, .b = -1);
    return OK;
}

void hash_abe_limpar(HashAbe *h)
{
    int i;

    for (i = 0; i < h->m; i++) {
        if (h->estado[i] != VAZIA) {
            h->estado[i] = VAZIA;
            TR(EV_ARR_MARK, .a = i, .b = TAG_NENHUMA);
        }
    }

    if (h->n > 0) {
        TR(EV_COUNT, .a = CNT_TAMANHO, .b = -h->n);
    }
    h->n = 0;
    h->tumulos = 0;

    /* O vetor volta a ser desenhado do zero: sem isto as células continuariam
     * mostrando os valores antigos, que não estão mais na tabela. */
    TR(EV_ARR_INIT, .a = h->m);
}

int hash_abe_tamanho(const HashAbe *h)
{
    return h->n;
}

int hash_abe_baldes(const HashAbe *h)
{
    return h->m;
}

int hash_abe_tumulos(const HashAbe *h)
{
    return h->tumulos;
}

int hash_abe_contem(const HashAbe *h, elem_t valor)
{
    int k;

    for (k = 0; k < h->m; k++) {
        int i = celula(h, valor, k);

        if (h->estado[i] == VAZIA) {
            return 0;
        }
        if (h->estado[i] == OCUPADA && h->chaves[i] == valor) {
            return 1;
        }
    }
    return 0;
}

void hash_abe_destruir(HashAbe *h)
{
    if (h == NULL) {
        return;
    }
    free(h->chaves);
    free(h->estado);
    free(h);
}

/* ---- adaptação para o vtable ------------------------------------------- *
 * Três tabelas sobre o mesmo código. Todos os ponteiros são iguais — o que
 * muda é o `sondagem` guardado na estrutura, decidido em `criar`.           */

static void *criar_com(int m, int sondagem)
{
    return hash_abe_criar(m, sondagem);
}

static void *vt_criar_linear(int capacidade)
{
    return criar_com(capacidade, TIPO_HASH_LINEAR);
}

static void *vt_criar_quad(int capacidade)
{
    return criar_com(capacidade, TIPO_HASH_QUAD);
}

static void *vt_criar_duplo(int capacidade)
{
    return criar_com(capacidade, TIPO_HASH_DUPLO);
}

static void vt_destruir(void *s)
{
    hash_abe_destruir(s);
}

static int vt_inserir(void *s, elem_t valor)
{
    return hash_abe_inserir(s, valor);
}

static void vt_limpar(void *s)
{
    hash_abe_limpar(s);
}

static int vt_tamanho(const void *s)
{
    return hash_abe_tamanho(s);
}

static int vt_capacidade(const void *s)
{
    return hash_abe_baldes(s);
}

static int vt_buscar(const void *s, elem_t valor, int *pos)
{
    return hash_abe_buscar(s, valor, pos);
}

static int vt_remover_valor(void *s, elem_t valor)
{
    return hash_abe_remover(s, valor);
}

#define TABELA_ABERTA(criar_fn)          \
    {                                    \
        .criar = (criar_fn),             \
        .destruir = vt_destruir,         \
        .inserir = vt_inserir,           \
        .limpar = vt_limpar,             \
        .tamanho = vt_tamanho,           \
        .capacidade = vt_capacidade,     \
        .buscar = vt_buscar,             \
        .remover_valor = vt_remover_valor \
    }

const TAD_Linear HASH_LINEAR = TABELA_ABERTA(vt_criar_linear);
const TAD_Linear HASH_QUAD = TABELA_ABERTA(vt_criar_quad);
const TAD_Linear HASH_DUPLO = TABELA_ABERTA(vt_criar_duplo);
