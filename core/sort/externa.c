/* core/sort/externa.c — intercalação externa: ordenar o que não cabe na
 * memória.
 *
 * Os seis algoritmos ao lado assumem uma coisa que este não pode assumir: que
 * o vetor inteiro está na RAM, e que tocar em v[i] é de graça. Aqui o vetor
 * está NO DISCO, e o que cabe na memória são `k` registros — nem um a mais. É
 * a mesma troca da árvore B, do outro lado da ementa: quando o acesso é caro,
 * o que se conta deixa de ser comparação e passa a ser PÁGINA.
 *
 * O algoritmo tem duas fases, e a segunda é a que dá nome a ele:
 *
 *   1. GERAÇÃO DOS RUNS. Traz um bloco de k registros para a memória, ordena
 *      esse bloco ali dentro, e o devolve ao disco. No fim, o arquivo não está
 *      ordenado — ele é uma sequência de ⌈n/k⌉ TRECHOS ordenados.
 *   2. INTERCALAÇÃO. Junta os runs dois a dois, dobrando o tamanho a cada
 *      passada, até sobrar um. Intercalar dois trechos ordenados não precisa
 *      de memória nenhuma além de um registro de cada lado — e é exatamente
 *      por isso que este é o algoritmo da memória secundária.
 *
 * O número que importa é a PASSADA. Cada uma lê o arquivo inteiro e o escreve
 * inteiro, e são 1 + ⌈log₂(n/k)⌉ delas. Dobrar a memória tira uma passada, e
 * é essa a lição: aqui, memória não deixa o algoritmo um pouco mais rápido —
 * ela tira uma varredura completa do disco.
 *
 * A página é o bloco de k registros, e ler o registro i só custa quando ele é
 * o PRIMEIRO do bloco: os outros k-1 vieram na mesma leitura. É por isso que
 * os runs terem tamanho múltiplo de k não é detalhe — é o que faz cada página
 * ser lida uma vez por passada, e não duas.
 *
 * Duas simplificações deliberadas, pelo mesmo motivo de o paginador não ter
 * cache: a memória real de uma intercalação binária se divide em três buffers
 * (dois de entrada e um de saída), e a implementação de verdade troca os
 * nomes das duas fitas em vez de copiar a saída de volta. Aqui a saída volta
 * para o vetor no fim de cada passada, porque é o vetor que está na tela e
 * uma fita que troca de nome no meio da animação não ensinaria nada. Nenhuma
 * das duas muda o número de passadas, que é o que se veio medir. */

#define TR_SRC SRC_EXTERNA

#include "ds/ordenacao.h"

#include <stdlib.h>

#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/paginador.h"
#include "ds/trace.h"

#include "acessos.h"
#include "passos.h"

/* Quantos registros cabem na memória, quando ninguém escolhe.
 *
 * Oito é pequeno de propósito: com o n de dezenas que a aba anima, uma
 * memória grande demais resolveria tudo na primeira fase e não haveria
 * intercalação para ver. */
enum { MEMORIA_PADRAO = 8, MEMORIA_MIN = 2 };

/* Estado de módulo, como o de medida.c e pelo mesmo motivo: a assinatura de
 * OrdenaFn é (vetor, n), e enfiar um terceiro parâmetro nela obrigaria os
 * outros seis algoritmos a receber um argumento que não usam. */
static int       g_memoria = MEMORIA_PADRAO;
static Paginador g_pag;
static int       g_passadas;

void externa_memoria(int k)
{
    g_memoria = (k < MEMORIA_MIN) ? MEMORIA_MIN : k;
}

int externa_memoria_atual(void)
{
    return g_memoria;
}

long externa_leituras(void)
{
    return paginador_leituras(&g_pag);
}

long externa_escritas(void)
{
    return paginador_escritas(&g_pag);
}

int externa_passadas(void)
{
    return g_passadas;
}

/* ---- o disco -------------------------------------------------------------
 *
 * A página é o bloco de k registros. Os dois ajudantes abaixo são a única
 * diferença entre este arquivo e um mergesort comum, e é neles que está a
 * lição inteira: o custo não é por registro, é por bloco.                   */

/* A página 0 fica reservada para "nenhuma", como no paginador. */
static int pagina_de(int i, int k)
{
    return i / k + 1;
}

/* Um fluxo sequencial sobre o arquivo, e a página que ele tem em mãos.
 *
 * A lembrança não é enfeite: durante a intercalação, um registro é comparado
 * contra vários do outro run antes de sair, e o cursor dele fica parado. Sem
 * guardar qual página já veio, cada comparação releria o mesmo bloco, e o
 * contador diria que a intercalação custa O(n) páginas quando ela custa
 * O(n/k). É o erro que faria o número desta aba mentir. */
typedef struct {
    int pagina;     /* 0 = nenhuma ainda */
} Fita;

/* Trazer o registro i para a memória. Custa uma leitura só quando ele está
 * num bloco que ainda não veio — os outros k-1 vieram junto, e é isso que a
 * memória compra. */
static void trazer(Fita *f, int i, int k)
{
    int p = pagina_de(i, k);

    if (p != f->pagina) {
        f->pagina = p;
        LER_PAGINA(&g_pag, p);
    }
}

/* Gravar o registro i. Mesma regra, do outro lado: o bloco vai para o disco
 * inteiro, quando fecha. */
static void gravar(Fita *f, int i, int k)
{
    int p = pagina_de(i, k);

    if (p != f->pagina) {
        f->pagina = p;
        ESCREVER_PAGINA(&g_pag, p);
    }
}

/* ---- fase 1: geração dos runs -------------------------------------------
 *
 * O bloco vem para a memória de uma vez, é ordenado ali dentro, e volta. A
 * ordenação de dentro é a INSERÇÃO, e não por acaso: k é pequeno — é o que
 * cabe na memória —, e em vetor pequeno a inserção ganha de todo mundo. É a
 * mesma inserção de core/sort/insercao.c, sobre um trecho.                  */

/* Índices absolutos de propósito: é o vetor da tela que eles apontam. Chamar
 * insercao_ordenar(v + lo, hi - lo + 1) daria o mesmo vetor ordenado e
 * animaria as células erradas, porque lá dentro o primeiro índice é 0. */
static void ordenar_bloco(elem_t *v, int lo, int hi)
{
    int i;

    for (i = lo + 1; i <= hi; i++) {
        elem_t chave = v[i];
        int    j = i - 1;

        TR(EV_PTR_SET, .a = PTR_I, .b = i);
        TR(EV_ARR_READ, .a = i);
        NA_MAO(chave);

        while (j >= lo) {
            COMPARAR_MAO(j);
            if (v[j] <= chave) {
                break;
            }
            ESCREVER(v, j + 1, v[j]);
            j--;
        }
        ESCREVER(v, j + 1, chave);
    }
}

static void gerar_runs(elem_t *v, int n, int k)
{
    int lo;

    /* Uma célula de auxiliar: o valor em mãos da inserção. Na fase 2 este
     * mesmo auxiliar vira a fita de saída, e cresce para n. */
    TR(EV_AUX_INIT, .a = 1);
    TR(EV_PHASE, .a = STR_GERANDO_RUNS, .b = k);

    for (lo = 0; lo < n; lo += k) {
        int hi = lo + k - 1;

        if (hi > n - 1) {
            hi = n - 1;
        }

        TR(EV_ARR_RANGE, .a = lo, .b = hi);

        /* O bloco inteiro numa leitura só. Daqui até a escrita, tudo o que
         * acontece é na memória, e é de graça perto de uma página. */
        LER_PAGINA(&g_pag, pagina_de(lo, k));
        ordenar_bloco(v, lo, hi);
        ESCREVER_PAGINA(&g_pag, pagina_de(lo, k));
    }

    g_passadas++;
    TR(EV_COUNT, .a = CNT_PASSADAS, .b = +1);
}

/* ---- fase 2: as passadas de intercalação --------------------------------
 *
 * Junta v[lo..meio] e v[meio+1..hi], que já estão ordenados, escrevendo o
 * resultado na fita de saída. É o mesmo laço do mergesort — a diferença é que
 * cada leitura e cada escrita passam pelos ajudantes de página.             */
static void intercalar(const elem_t *v, elem_t *saida, int lo, int meio, int hi,
                       int k)
{
    /* Três fitas, que é exatamente o que a intercalação binária precisa: duas
     * de entrada e uma de saída. */
    Fita esq = { 0 };
    Fita dir = { 0 };
    Fita out = { 0 };

    int i = lo;
    int j = meio + 1;
    int s = lo;

    TR(EV_PHASE, .a = STR_INTERCALANDO, .b = lo, .c = hi);
    TR(EV_ARR_RANGE, .a = lo, .b = hi);

    while (i <= meio && j <= hi) {
        TR(EV_PTR_SET, .a = PTR_I, .b = i);
        TR(EV_PTR_SET, .a = PTR_J, .b = j);
        trazer(&esq, i, k);
        trazer(&dir, j, k);
        gravar(&out, s, k);
        COMPARAR(i, j);

        /* O <= mantém a estabilidade, como no mergesort interno: com valores
         * iguais, o do run da esquerda sai primeiro. */
        if (v[i] <= v[j]) {
            ESCREVER_AUX(saida, s, v[i]);
            i++;
        } else {
            ESCREVER_AUX(saida, s, v[j]);
            j++;
        }
        s++;
    }

    /* Um dos runs acabou; o resto do outro já está em ordem e só precisa ser
     * copiado — e continua custando página, porque continua passando pelo
     * disco. */
    while (i <= meio) {
        trazer(&esq, i, k);
        gravar(&out, s, k);
        ESCREVER_AUX(saida, s, v[i]);
        i++;
        s++;
    }
    while (j <= hi) {
        trazer(&dir, j, k);
        gravar(&out, s, k);
        ESCREVER_AUX(saida, s, v[j]);
        j++;
        s++;
    }
}

/* Um run sem par: o último, quando a contagem é ímpar. Ele atravessa a
 * passada inteiro, sem ninguém para intercalar com ele — e ainda assim é lido
 * e escrito, porque a passada varre o arquivo todo. */
static void copiar_run(const elem_t *v, elem_t *saida, int lo, int hi, int k)
{
    Fita entrada = { 0 };
    Fita out = { 0 };
    int  i;

    TR(EV_ARR_RANGE, .a = lo, .b = hi);
    for (i = lo; i <= hi; i++) {
        trazer(&entrada, i, k);
        gravar(&out, i, k);
        ESCREVER_AUX(saida, i, v[i]);
    }
}

int externa_ordenar(elem_t *v, int n)
{
    elem_t *saida;
    int     k = g_memoria;
    int     tamanho;
    int     passada = 0;

    paginador_iniciar(&g_pag);
    g_passadas = 0;

    if (n <= 1) {
        return OK;
    }

    /* Memória maior que o arquivo é o caso degenerado, e é bom que ele
     * apareça: o arquivo inteiro cabe na RAM, a fase 1 ordena tudo de uma
     * vez, e não sobra intercalação nenhuma para fazer. */
    if (k > n) {
        k = n;
    }

    saida = malloc((size_t) n * sizeof *saida);
    if (saida == NULL) {
        return ERR_SEM_MEMORIA;
    }
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = +1);

    gerar_runs(v, n, k);

    /* A fita de saída, agora do tamanho do arquivo. */
    TR(EV_AUX_INIT, .a = n);

    for (tamanho = k; tamanho < n; tamanho *= 2) {
        int lo;

        passada++;
        TR(EV_PHASE, .a = STR_PASSADA, .b = passada, .c = tamanho * 2);

        for (lo = 0; lo < n; lo += 2 * tamanho) {
            int meio = lo + tamanho - 1;
            int hi = lo + 2 * tamanho - 1;

            if (hi > n - 1) {
                hi = n - 1;
            }
            if (meio >= hi) {
                copiar_run(v, saida, lo, hi, k);
            } else {
                intercalar(v, saida, lo, meio, hi, k);
            }
        }

        /* A saída vira a entrada da próxima passada. Numa implementação de
         * verdade isto é trocar o nome de duas fitas e não custa nada; aqui a
         * cópia existe para o vetor da tela continuar sendo o arquivo, e por
         * isso ela não conta página. */
        for (lo = 0; lo < n; lo++) {
            ESCREVER(v, lo, saida[lo]);
        }

        g_passadas++;
        TR(EV_COUNT, .a = CNT_PASSADAS, .b = +1);
    }

    free(saida);
    TR(EV_COUNT, .a = CNT_ALOCACOES, .b = -1);
    TR(EV_ARR_RANGE, .a = 0, .b = n - 1);
    return OK;
}
