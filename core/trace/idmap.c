/* core/trace/idmap.c — tabela ponteiro -> id estável.
 *
 * Endereçamento aberto com sondagem linear, capacidade sempre potência de
 * dois. A remoção precisa de túmulo: apagar um slot no meio de uma cadeia de
 * sondagem tornaria invisível tudo que veio depois dela. */

#include "ds/idmap.h"

#include <stdlib.h>

typedef struct {
    const void *chave;
    int32_t     id;
} entrada;

/* Endereço de um objeto estático serve de marca de túmulo: é garantidamente
 * diferente de NULL e de qualquer ponteiro que o usuário venha a registrar. */
static const char g_marca;
#define TUMULO ((const void *) &g_marca)

#define CAP_INICIAL 256u

static entrada *g_tab;
static uint32_t g_cap;          /* potência de dois, ou 0 se ainda não alocou */
static uint32_t g_vivos;        /* entradas de verdade                        */
static uint32_t g_usados;       /* vivos + túmulos — é isto que enche a tabela*/
static int32_t  g_proximo = 1;  /* 0 é reservado para NULL                    */
static int      g_falhou;

/* Mistura à la MurmurHash3. Os bits baixos de um ponteiro são quase sempre
 * zero por causa do alinhamento; sem espalhar, a sondagem linear degenera.
 * O cálculo é feito em 64 bits de propósito: no wasm32 o ponteiro tem 32. */
static uint32_t espalhar(const void *p, uint32_t cap)
{
    uint64_t x = (uint64_t) (uintptr_t) p;

    x ^= x >> 33;
    x *= UINT64_C(0xff51afd7ed558ccd);
    x ^= x >> 33;
    x *= UINT64_C(0xc4ceb9fe1a85ec53);
    x ^= x >> 33;

    return (uint32_t) x & (cap - 1u);
}

/* Índice do slot de `p`, ou do primeiro slot livre onde ele caberia.
 * Devolve 0 se a tabela estiver cheia sem encontrá-lo. */
static int procurar(const entrada *tab, uint32_t cap, const void *p,
                    uint32_t *saida)
{
    uint32_t i = espalhar(p, cap);
    uint32_t percorridos = 0;
    int      achou_tumulo = 0;
    uint32_t primeiro_tumulo = 0;

    while (percorridos < cap) {
        const void *k = tab[i].chave;

        if (k == p) {
            *saida = i;
            return 1;                   /* achou de fato                  */
        }
        if (k == NULL) {
            /* Vazio encerra a cadeia: se havia um túmulo antes, reaproveita. */
            *saida = achou_tumulo ? primeiro_tumulo : i;
            return 0;
        }
        if (k == TUMULO && !achou_tumulo) {
            achou_tumulo = 1;
            primeiro_tumulo = i;
        }

        i = (i + 1u) & (cap - 1u);
        percorridos++;
    }

    if (achou_tumulo) {
        *saida = primeiro_tumulo;
        return 0;
    }
    return -1;                          /* cheia e sem o ponteiro         */
}

/* Realoca para `nova_cap` e reinsere só as entradas vivas, o que de quebra
 * limpa os túmulos. */
static int reconstruir(uint32_t nova_cap)
{
    entrada *nova = calloc(nova_cap, sizeof *nova);
    uint32_t i;

    if (nova == NULL) {
        return 0;
    }

    for (i = 0; i < g_cap; i++) {
        const void *k = g_tab[i].chave;
        uint32_t    destino;

        if (k == NULL || k == TUMULO) {
            continue;
        }
        (void) procurar(nova, nova_cap, k, &destino);
        nova[destino] = g_tab[i];
    }

    free(g_tab);
    g_tab = nova;
    g_cap = nova_cap;
    g_usados = g_vivos;
    return 1;
}

/* Garante que cabe mais uma entrada, mantendo a ocupação abaixo de 1/2.
 * Se só houver túmulos sobrando, reconstrói no mesmo tamanho. */
static int garantir_espaco(void)
{
    if (g_cap == 0u) {
        return reconstruir(CAP_INICIAL);
    }
    if ((g_usados + 1u) * 2u <= g_cap) {
        return 1;
    }
    return reconstruir((g_vivos + 1u) * 2u > g_cap ? g_cap * 2u : g_cap);
}

int32_t id_de(const void *p)
{
    uint32_t onde;
    int      achou;

    if (p == NULL) {
        return 0;
    }

    if (g_cap > 0u) {
        achou = procurar(g_tab, g_cap, p, &onde);
        if (achou == 1) {
            return g_tab[onde].id;
        }
    }

    if (!garantir_espaco()) {
        /* Sem memória para registrar. O id sai único, para nada colidir,
         * mas não é estável: uma segunda chamada com o mesmo ponteiro daria
         * outro. A flag deixa isso visível em vez de virar bug mudo. */
        g_falhou = 1;
        return g_proximo++;
    }

    achou = procurar(g_tab, g_cap, p, &onde);
    if (achou != 0) {
        g_falhou = 1;
        return g_proximo++;
    }

    if (g_tab[onde].chave == NULL) {
        g_usados++;             /* túmulo reaproveitado não consome espaço */
    }
    g_tab[onde].chave = p;
    g_tab[onde].id = g_proximo++;
    g_vivos++;
    return g_tab[onde].id;
}

void id_esquece(const void *p)
{
    uint32_t onde;

    if (p == NULL || g_cap == 0u) {
        return;
    }
    if (procurar(g_tab, g_cap, p, &onde) != 1) {
        return;
    }

    g_tab[onde].chave = TUMULO;
    g_tab[onde].id = 0;
    g_vivos--;
}

void idmap_reset(void)
{
    free(g_tab);
    g_tab = NULL;
    g_cap = 0u;
    g_vivos = 0u;
    g_usados = 0u;
    g_proximo = 1;
    g_falhou = 0;
}

int32_t idmap_vivos(void)
{
    return (int32_t) g_vivos;
}

int idmap_falhou(void)
{
    return g_falhou;
}
