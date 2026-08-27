/* core/include/ds/ids.h — os enums que o C e o TypeScript compartilham.
 *
 * tools/gen_enums.py lê este arquivo e gera web/src/core/ops.ts. Manter as
 * duas listas na mão dessincroniza mais cedo ou mais tarde, e o sintoma é
 * mudo: a animação faz a coisa errada e nada quebra.
 *
 * Formato esperado pelo gerador: um `typedef enum { ... } nome;` por bloco,
 * um identificador por linha, sem inicializador explícito — a ordem é que
 * define o valor, nos dois lados. */

#ifndef DS_IDS_H
#define DS_IDS_H

/* ---------------------------------------------------------------------------
 * Tipo do evento.
 *
 * O vocabulário é pequeno de propósito e serve a todas as estruturas. Antes
 * de acrescentar um evento, tente compor os que já existem: EV_PTR_SET vale
 * tanto para "topo aponta para o nó 7" quanto para "início vale 5".
 * ------------------------------------------------------------------------- */
typedef enum {
    /* genéricos */
    EV_MSG,             /* a = STR_*                                       */
    EV_COUNT,           /* a = CNT_*, b = delta                            */
    EV_PHASE,           /* a = STR_* da fase corrente                      */

    /* mundo "vetor": ordenação, busca, hash aberto */
    EV_ARR_INIT,        /* a = n                                           */
    EV_ARR_READ,        /* a = i                                           */
    EV_ARR_COMPARE,     /* a = i, b = j; c = 1 -> b é índice no auxiliar    *
                         * O c = 1 é o "elemento em mãos" da inserção e do   *
                         * shell: eles tiram um valor do vetor e comparam    *
                         * contra ele. Um evento novo para isso seria o      *
                         * mesmo evento com outro nome — o auxiliar já       *
                         * existe no vocabulário, e é onde o valor está.     */
    EV_ARR_SWAP,        /* a = i, b = j                                    */
    EV_ARR_WRITE,       /* a = i, b = valor                                */
    EV_ARR_RANGE,       /* a = lo, b = hi — destaca o subvetor ativo       */
    EV_ARR_MARK,        /* a = i, b = TAG_*                                */
    EV_AUX_INIT,        /* a = n — buffer auxiliar do merge                */
    EV_AUX_WRITE,       /* a = i, b = valor                                */

    /* mundo "grafo de nós": listas, árvores */
    EV_NODE_NEW,        /* a = id, b = valor                               */
    EV_NODE_FREE,       /* a = id                                          */
    EV_NODE_SET,        /* a = id, b = slot da chave, c = valor            */
    EV_EDGE_SET,        /* a = id origem, b = slot, c = id destino         */
    EV_PTR_SET,         /* a = PTR_*, b = id do nó ou índice               */
    EV_VISIT,           /* a = id — nó sob o cursor                        */
    EV_UNVISIT,         /* a = id                                          */

    /* memória secundária */
    EV_DISK_READ,       /* a = número da página                            */
    EV_DISK_WRITE,      /* a = número da página                            */

    EV_KIND_COUNT
} ev_kind;

/* ---------------------------------------------------------------------------
 * Arquivo-fonte que emitiu o evento. Cada .c instrumentado abre com
 * `#define TR_SRC SRC_<ARQUIVO>`, e o frontend usa o id para escolher qual
 * código-fonte exibir no painel lateral.
 * ------------------------------------------------------------------------- */
typedef enum {
    SRC_NENHUM,
    SRC_API,
    SRC_PILHA_ENC,
    SRC_PILHA_VET,
    SRC_FILA_ENC,
    SRC_FILA_VET,
    SRC_LISTA_SIMPLES,
    SRC_LISTA_DUPLA,
    SRC_LISTA_CIRCULAR,
    SRC_BOLHA,
    SRC_SELECAO,
    SRC_INSERCAO,
    SRC_SHELL,
    SRC_QUICK,
    SRC_MERGE,
    SRC_CENA,
    SRC_COUNT
} ds_src;

/* ---------------------------------------------------------------------------
 * Mensagens. O C emite o id; a frase vive em web/src/i18n/. Cada STR_ daqui
 * precisa de uma chave em pt.ts e en.ts, e o CI falha se faltar.
 * ------------------------------------------------------------------------- */
typedef enum {
    STR_NENHUMA,
    STR_PING,
    STR_PILHA_VAZIA,
    STR_PILHA_CHEIA,
    STR_FILA_VAZIA,
    STR_FILA_CHEIA,
    STR_DEU_VOLTA,
    STR_LISTA_VAZIA,
    STR_POSICAO_INVALIDA,
    STR_ACHOU,
    STR_NAO_ACHOU,
    STR_ANDANDO,        /* percorrendo a lista até a posição pedida         */

    /* ordenação. A fase é o que o algoritmo está fazendo agora, e é o que
     * separa um vetor piscando de um algoritmo com etapas. */
    STR_ORDENADO,
    STR_SEM_TROCAS,     /* a bolha passou inteira sem trocar: pode parar    */
    STR_PASSADA,
    STR_PROCURANDO_MIN,
    STR_DESLOCANDO,     /* inserção abrindo espaço à direita                */
    STR_GAP,            /* shellsort: a passada de um gap                   */
    STR_PARTICIONANDO,
    STR_DIVIDINDO,
    STR_INTERCALANDO,
    STR_COUNT
} ds_str;

/* ---------------------------------------------------------------------------
 * Ponteiros nomeados desenhados na tela (o rótulo "topo", "frente", "fim").
 *
 * O alvo de EV_PTR_SET muda de significado conforme o mundo, e o renderizador
 * decide pelo tipo da sessão:
 *
 *   encadeada   b = id de nó, e 0 é NULL
 *   com vetor   b = índice, e -1 é "nenhum" (0 é um índice válido)
 *
 * É essa dupla leitura que dispensa criar um evento por implementação.
 * ------------------------------------------------------------------------- */
typedef enum {
    PTR_NENHUM,
    PTR_TOPO,
    PTR_FRENTE,
    PTR_FIM,
    PTR_INICIO,
    PTR_CURSOR,         /* onde a travessia está agora                      */

    /* Os dois índices que todo algoritmo de ordenação tem, e o terceiro que
     * a seleção precisa. Ficam aqui, e não num evento novo, porque índice com
     * nome é exatamente o que EV_PTR_SET já carrega. */
    PTR_I,
    PTR_J,
    PTR_MIN,
    PTR_COUNT
} ds_ptr;

/* ---------------------------------------------------------------------------
 * Contadores exibidos no painel de métricas.
 * ------------------------------------------------------------------------- */
typedef enum {
    CNT_TAMANHO,
    CNT_COMPARACOES,
    CNT_ESCRITAS,
    CNT_ALOCACOES,
    CNT_COUNT
} ds_cnt;

/* ---------------------------------------------------------------------------
 * Marcas de EV_ARR_MARK.
 * ------------------------------------------------------------------------- */
typedef enum {
    TAG_NENHUMA,
    TAG_ORDENADO,
    TAG_PIVO,
    TAG_LIVRE,          /* célula que já foi ocupada e não é mais           */
    TAG_COUNT
} ds_tag;

/* ---------------------------------------------------------------------------
 * Operações aceitas por ds_call(op, a, b, c).
 * ------------------------------------------------------------------------- */
typedef enum {
    OP_PING,            /* só emite EV_MSG — prova o caminho C -> wasm -> JS */
    OP_PUSH,            /* a = valor                                         */
    OP_POP,
    OP_TOPO,
    OP_LIMPAR,
    OP_INSERIR_EM,      /* a = valor, b = posição                            */
    OP_REMOVER_EM,      /* b = posição                                       */
    OP_BUSCAR,          /* a = valor                                         */
    OP_GERAR,           /* a = n, b = DIST_*, c = semente                    */
    OP_ORDENAR,         /* a = ALG_*                                         */
    OP_COUNT
} ds_op;

/* ---------------------------------------------------------------------------
 * Que estrutura a sessão está simulando. O frontend usa isto para escolher o
 * renderizador: o mesmo EV_PTR_SET aponta para um nó na versão encadeada e
 * para um índice na versão com vetor.
 * ------------------------------------------------------------------------- */
typedef enum {
    TIPO_NENHUM,
    TIPO_PILHA_ENC,
    TIPO_PILHA_VET,
    TIPO_FILA_ENC,
    TIPO_FILA_VET,      /* a com vetor é a circular                         */
    TIPO_LISTA_SIMPLES,
    TIPO_LISTA_DUPLA,
    TIPO_LISTA_CIRCULAR,
    TIPO_ORDENACAO,     /* a sessão é um vetor a ordenar, não um TAD        */
    TIPO_COUNT
} ds_tipo;

/* ---------------------------------------------------------------------------
 * Algoritmos de ordenação.
 *
 * A ordem é a da aula — os três quadráticos, depois os que melhoram o pior
 * caso —, e é ela que o seletor da aba usa. Acrescentar um algoritmo aqui e
 * na tabela de core/sort/ordenacao.c basta: a interface se monta sozinha.
 * ------------------------------------------------------------------------- */
typedef enum {
    ALG_BOLHA,
    ALG_SELECAO,
    ALG_INSERCAO,
    ALG_SHELL,
    ALG_QUICK,
    ALG_MERGE,
    ALG_COUNT
} ds_alg;

/* ---------------------------------------------------------------------------
 * Distribuição do vetor inicial.
 *
 * Não é enfeite: é o que mostra que complexidade média e pior caso são coisas
 * diferentes. POUCOS_DISTINTOS existe para matar o quicksort de partição
 * ingênua, e QUASE_ORDENADO para a inserção ganhar do quicksort na tela.
 *
 * MANUAL lê os valores do buffer de entrada (ds_buffer), único caminho para
 * um dado que não cabe nos quatro inteiros de ds_call.
 * ------------------------------------------------------------------------- */
typedef enum {
    DIST_ALEATORIO,
    DIST_QUASE_ORDENADO,
    DIST_INVERSO,
    DIST_POUCOS_DISTINTOS,
    DIST_ORDENADO,
    DIST_MANUAL,
    DIST_COUNT
} ds_dist;

#endif /* DS_IDS_H */
