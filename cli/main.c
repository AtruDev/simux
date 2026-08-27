/* cli/main.c — binário de terminal sobre o mesmo core que vai para o wasm.
 *
 * Roda um script de operações e despeja o trace em texto, sem navegador no
 * meio. É a ferramenta de depuração do projeto: quando a animação parecer
 * errada, o primeiro passo é reproduzir a mesma sequência aqui e ler os
 * eventos crus.
 *
 *   cli                          cena curta com a pilha encadeada
 *   cli fila_vet 4 i1 i2 i3 r c  fila circular de capacidade 4
 *   cli ordenacao 12 g0 o4       quicksort sobre doze valores aleatórios
 *
 *   i<valor>  inserir      r  remover      c  consultar      l  limpar
 *   b<valor>  buscar      x<valor>  remover por valor (árvore)
 *   p<ordem>  percurso: 0 em ordem, 1 pré-ordem, 2 pós-ordem
 *   g<dist>   gerar a cena da aba de ordenação (semente 1)
 *   o<alg>    ordenar com ALG_<alg>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds/api.h"
#include "ds/erros.h"
#include "ds/ids.h"

static const char *NOME_EVENTO[] = {
    "MSG", "COUNT", "PHASE",
    "ARR_INIT", "ARR_READ", "ARR_COMPARE", "ARR_SWAP", "ARR_WRITE",
    "ARR_RANGE", "ARR_MARK", "AUX_INIT", "AUX_WRITE",
    "NODE_NEW", "NODE_FREE", "NODE_SET", "EDGE_SET", "PTR_SET",
    "VISIT", "UNVISIT", "DISK_READ", "DISK_WRITE",
};

struct Nomeado {
    const char *nome;
    int32_t     tipo;
};

static const struct Nomeado TIPOS[] = {
    { "pilha_enc", TIPO_PILHA_ENC },
    { "pilha_vet", TIPO_PILHA_VET },
    { "fila_enc",  TIPO_FILA_ENC  },
    { "fila_vet",  TIPO_FILA_VET  },
    { "lista",     TIPO_LISTA_SIMPLES  },
    { "lista_dupla",    TIPO_LISTA_DUPLA    },
    { "lista_circular", TIPO_LISTA_CIRCULAR },
    { "ordenacao", TIPO_ORDENACAO },
    { "busca_seq", TIPO_BUSCA_SEQ },
    { "busca_bin", TIPO_BUSCA_BIN },
    { "abb",       TIPO_ABB       },
    { "avl",       TIPO_AVL       },
};

static int32_t tipo_por_nome(const char *nome)
{
    size_t i;

    for (i = 0; i < sizeof TIPOS / sizeof TIPOS[0]; i++) {
        if (strcmp(TIPOS[i].nome, nome) == 0) {
            return TIPOS[i].tipo;
        }
    }
    return -1;
}

static void despejar(const char *rotulo, int32_t rc)
{
    const ev_t *eventos = ds_trace_ptr();
    int32_t     n = ds_trace_len();
    int32_t     i;

    printf("%-12s %2d evento(s)%s%s\n", rotulo, (int) n,
           rc < 0 ? "  [erro ]" : "",
           ds_trace_truncado() ? "  (truncado)" : "");

    for (i = 0; i < n; i++) {
        const ev_t *e = &eventos[i];
        const char *nome = (e->kind >= 0 && e->kind < EV_KIND_COUNT)
                           ? NOME_EVENTO[e->kind] : "?";

        printf("    %-11s src=%d linha=%-4d a=%-5d b=%-5d c=%d\n",
               nome, (int) e->src, (int) e->line,
               (int) e->a, (int) e->b, (int) e->c);
    }
}

static void executar(const char *passo)
{
    char rotulo[32];

    switch (passo[0]) {
    case 'i':
        snprintf(rotulo, sizeof rotulo, "inserir %s", passo + 1);
        despejar(rotulo, ds_call(OP_PUSH, atoi(passo + 1), 0, 0));
        break;
    case 'r':
        despejar("remover", ds_call(OP_POP, 0, 0, 0));
        break;
    case 'c':
        despejar("consultar", ds_call(OP_TOPO, 0, 0, 0));
        break;
    case 'l':
        despejar("limpar", ds_call(OP_LIMPAR, 0, 0, 0));
        break;
    case 'b':
        snprintf(rotulo, sizeof rotulo, "buscar %s", passo + 1);
        despejar(rotulo, ds_call(OP_BUSCAR, atoi(passo + 1), 0, 0));
        break;
    case 'x':
        snprintf(rotulo, sizeof rotulo, "remover %s", passo + 1);
        despejar(rotulo, ds_call(OP_REMOVER_VALOR, atoi(passo + 1), 0, 0));
        break;
    case 'p':
        snprintf(rotulo, sizeof rotulo, "percurso %s", passo + 1);
        despejar(rotulo, ds_call(OP_PERCURSO, atoi(passo + 1), 0, 0));
        break;
    case 'g':
        /* O tamanho é a capacidade da sessão: gerar menos que o vetor inteiro
         * é caso da interface, e aqui só atrapalharia. */
        snprintf(rotulo, sizeof rotulo, "gerar dist=%s", passo + 1);
        despejar(rotulo, ds_call(OP_GERAR, ds_capacidade(),
                                 atoi(passo + 1), 1));
        break;
    case 'o':
        snprintf(rotulo, sizeof rotulo, "ordenar alg=%s", passo + 1);
        despejar(rotulo, ds_call(OP_ORDENAR, atoi(passo + 1), 0, 0));
        break;
    default:
        fprintf(stderr, "passo desconhecido: %s\n", passo);
        break;
    }
}

int main(int argc, char **argv)
{
    static const char *CENA[] = { "i10", "i20", "c", "r", "l", "r" };

    int32_t tipo = TIPO_PILHA_ENC;
    int32_t capacidade = 8;
    int     i;

    printf("simux %s\n\n", SIMUX_VERSAO);

    if (argc > 1) {
        tipo = tipo_por_nome(argv[1]);
        if (tipo < 0) {
            fprintf(stderr, "estrutura desconhecida: %s\n", argv[1]);
            return 2;
        }
    }
    if (argc > 2) {
        capacidade = atoi(argv[2]);
    }

    if (ds_sessao_nova(tipo, capacidade) != OK) {
        fprintf(stderr, "não foi possível abrir a sessão: %d\n",
                (int) ds_erro());
        return 1;
    }
    despejar("sessão", OK);

    if (argc > 3) {
        for (i = 3; i < argc; i++) {
            executar(argv[i]);
        }
    } else {
        for (i = 0; i < (int) (sizeof CENA / sizeof CENA[0]); i++) {
            executar(CENA[i]);
        }
    }

    ds_sessao_fim();
    return 0;
}
