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
    EV_ARR_COMPARE,     /* a = i, b = j                                    */
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
    TIPO_COUNT
} ds_tipo;

#endif /* DS_IDS_H */
