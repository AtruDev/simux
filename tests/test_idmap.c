/* tests/test_idmap.c — a tabela ponteiro -> id.
 *
 * O caso que importa é o endereço reciclado. Sem ele, o resto da tabela seria
 * complexidade sem motivo: bastaria usar o próprio ponteiro como id. */

#include <stdlib.h>

#include "ds/idmap.h"

#include "runner.h"

enum { N_CRESCIMENTO = 5000 };

static char g_blocos[N_CRESCIMENTO];

void suite_idmap(void)
{
    int     i;
    int     alvo, outro;
    int32_t id_alvo, id_outro, id_depois, primeiro;

    CASO("NULL e sempre 0");
    idmap_reset();
    ASSERT_EQ(id_de(NULL), 0);

    CASO("o id e estavel e nunca 0");
    id_alvo = id_de(&alvo);
    ASSERT_TRUE(id_alvo > 0);
    ASSERT_EQ(id_de(&alvo), id_alvo);
    ASSERT_EQ(id_de(&alvo), id_alvo);

    CASO("ponteiros distintos, ids distintos");
    id_outro = id_de(&outro);
    ASSERT_TRUE(id_outro != id_alvo);
    ASSERT_EQ(idmap_vivos(), 2);

    CASO("endereco reciclado ganha id novo");
    /* O ponto todo da tabela. Depois do esquecimento, o MESMO endereço tem
     * que dar um id inédito — senão um nó recém-criado herdaria a identidade
     * visual de um nó morto, e a animação moveria o nó errado. */
    id_esquece(&alvo);
    ASSERT_EQ(idmap_vivos(), 1);
    id_depois = id_de(&alvo);
    ASSERT_TRUE(id_depois != id_alvo);
    ASSERT_TRUE(id_depois > 0);
    /* e o vizinho não foi afetado pelo túmulo no meio da cadeia */
    ASSERT_EQ(id_de(&outro), id_outro);

    CASO("esquecer o que nao esta la nao faz nada");
    idmap_reset();
    id_esquece(&alvo);
    id_esquece(NULL);
    ASSERT_EQ(idmap_vivos(), 0);

    CASO("reciclagem de verdade, via malloc e free");
    {
        void   *p1, *p2;
        int32_t antes, depois;

        idmap_reset();
        p1 = malloc(64);
        ASSERT_TRUE(p1 != NULL);
        antes = id_de(p1);
        id_esquece(p1);
        free(p1);

        p2 = malloc(64);
        ASSERT_TRUE(p2 != NULL);
        depois = id_de(p2);
        /* O alocador pode ou não devolver o mesmo endereço; o teste vale nos
         * dois casos, e é justamente por não dar para prever que a tabela
         * existe. */
        ASSERT_TRUE(depois != antes);
        free(p2);
    }

    CASO("cresce alem da capacidade inicial sem perder ids");
    idmap_reset();
    primeiro = id_de(&g_blocos[0]);
    for (i = 1; i < N_CRESCIMENTO; i++) {
        (void) id_de(&g_blocos[i]);
    }
    ASSERT_EQ(idmap_vivos(), N_CRESCIMENTO);
    ASSERT_EQ(id_de(&g_blocos[0]), primeiro);
    ASSERT_EQ(id_de(&g_blocos[N_CRESCIMENTO - 1]), N_CRESCIMENTO);
    ASSERT_EQ(idmap_falhou(), 0);

    CASO("muitos tumulos nao escondem quem ficou");
    for (i = 0; i < N_CRESCIMENTO; i += 2) {
        id_esquece(&g_blocos[i]);
    }
    ASSERT_EQ(idmap_vivos(), N_CRESCIMENTO / 2);
    for (i = 1; i < N_CRESCIMENTO; i += 2) {
        ASSERT_TRUE(id_de(&g_blocos[i]) > 0);
    }
    ASSERT_EQ(idmap_vivos(), N_CRESCIMENTO / 2);

    CASO("reset zera a tabela e a numeracao");
    idmap_reset();
    ASSERT_EQ(idmap_vivos(), 0);
    ASSERT_EQ(idmap_falhou(), 0);
    ASSERT_EQ(id_de(&alvo), 1);
    idmap_reset();
}
