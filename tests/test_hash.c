/* tests/test_hash.c — as quatro tabelas hash.
 *
 * A invariante de uma tabela hash é uma frase, e é a única que ela tem: tudo o
 * que foi inserido é encontrado, e nada que foi removido é encontrado. Não há
 * ordem para verificar, nem forma — o desenho da tabela é acidente da função
 * hash, e conferi-lo seria testar o acidente.
 *
 * A parte que precisa de cuidado é a REMOÇÃO no endereçamento aberto. Apagar a
 * célula em vez de marcá-la como túmulo quebra a sondagem de quem veio depois,
 * e o sintoma é cruel: um elemento que ninguém removeu some da tabela. O teste
 * monta essa situação de propósito, porque um fuzz aleatório pode levar
 * milhares de operações para tropeçar nela. */

#include <stddef.h>

#include "ds/hash.h"
#include "ds/aleatorio.h"
#include "ds/erros.h"
#include "ds/ids.h"
#include "ds/idmap.h"
#include "ds/tipos.h"
#include "ds/trace.h"

#include "linear.h"
#include "runner.h"

/* Um m primo, para a sondagem dupla poder visitar a tabela inteira. */
enum { M_PRIMO = 31, VALORES = 200 };

static const int SONDAGENS[3] = {
    TIPO_HASH_LINEAR, TIPO_HASH_QUAD, TIPO_HASH_DUPLO
};

static const char *NOME_SONDAGEM[3] = { "linear", "quadrática", "dupla" };

/* ---- encadeado ---------------------------------------------------------- */

static void suite_encadeado(void)
{
    HashEnc *h = hash_enc_criar(8);
    int      balde = -1;
    int      i;

    CASO("hash encadeado: a colisão vira cadeia, e a tabela não enche");
    ASSERT_TRUE(h != NULL);
    ASSERT_EQ(hash_enc_tamanho(h), 0);
    ASSERT_EQ(hash_enc_baldes(h), 8);
    ASSERT_EQ(hash_enc_maior_cadeia(h), 0);

    /* 8, 16 e 24 caem todos no balde 0 com m = 8. É a colisão previsível que
     * a função h(k) = k mod m existe para dar. */
    ASSERT_EQ(hash_enc_inserir(h, 8), OK);
    ASSERT_EQ(hash_enc_inserir(h, 16), OK);
    ASSERT_EQ(hash_enc_inserir(h, 24), OK);
    ASSERT_EQ(hash_enc_tamanho(h), 3);
    ASSERT_EQ(hash_enc_maior_cadeia(h), 3);

    for (i = 8; i <= 24; i += 8) {
        ASSERT_EQ(hash_enc_buscar(h, i, &balde), OK);
        ASSERT_EQ(balde, 0);
    }

    CASO("hash encadeado: nunca enche, e passa de n > m sem reclamar");
    for (i = 0; i < 64; i++) {
        ASSERT_EQ(hash_enc_inserir(h, 100 + i), OK);
    }
    ASSERT_EQ(hash_enc_tamanho(h), 3 + 64);
    ASSERT_TRUE(hash_enc_maior_cadeia(h) >= 8);

    CASO("hash encadeado: repetido não entra");
    ASSERT_EQ(hash_enc_inserir(h, 8), OK);
    ASSERT_EQ(hash_enc_tamanho(h), 67);

    CASO("hash encadeado: remover da cabeça e do meio da cadeia");
    /* 24 entrou por último no balde 0, então é a cabeça; 16 está no meio. */
    ASSERT_EQ(hash_enc_remover(h, 24), OK);
    ASSERT_TRUE(!hash_enc_contem(h, 24));
    ASSERT_TRUE(hash_enc_contem(h, 16));
    ASSERT_TRUE(hash_enc_contem(h, 8));

    ASSERT_EQ(hash_enc_remover(h, 16), OK);
    ASSERT_TRUE(!hash_enc_contem(h, 16));
    ASSERT_TRUE(hash_enc_contem(h, 8));

    ASSERT_EQ(hash_enc_remover(h, 999), ERR_NAO_ENCONTRADO);

    CASO("hash encadeado: negativo não vira índice negativo");
    /* O `%` do C devolve negativo para chave negativa. Sem a correção, isto
     * é acesso fora do vetor — e o ASAN do CI pegaria, mas só se alguém
     * lembrasse de inserir um negativo. */
    ASSERT_EQ(hash_enc_inserir(h, -1), OK);
    ASSERT_EQ(hash_enc_inserir(h, -9), OK);
    ASSERT_TRUE(hash_enc_contem(h, -1));
    ASSERT_TRUE(hash_enc_contem(h, -9));
    ASSERT_EQ(hash_enc_remover(h, -9), OK);
    ASSERT_TRUE(!hash_enc_contem(h, -9));

    CASO("hash encadeado: limpar devolve a memória");
    idmap_reset();
    hash_enc_limpar(h);
    ASSERT_EQ(hash_enc_tamanho(h), 0);
    ASSERT_EQ(hash_enc_maior_cadeia(h), 0);
    ASSERT_EQ(idmap_vivos(), 0);
    ASSERT_EQ(hash_enc_inserir(h, 1), OK);
    ASSERT_TRUE(hash_enc_contem(h, 1));

    ASSERT_TRUE(hash_enc_criar(0) == NULL);
    hash_enc_destruir(h);
    hash_enc_destruir(NULL);
    idmap_reset();
}

/* ---- aberto: a remoção, que é onde mora o bug --------------------------- */

/* Monta a situação exata que quebra quem apaga em vez de enterrar.
 *
 * Com sondagem linear e m = 8: 8, 16 e 24 caem no balde 0 e vão parar em 0, 1
 * e 2. Removido o 16 — o do meio da sequência —, a busca por 24 tem que
 * ATRAVESSAR a célula 1. Se ela tivesse sido apagada, a busca pararia ali e o
 * 24 sumiria da tabela sem nunca ter sido removido. */
static void suite_tumulo(void)
{
    HashAbe *h = hash_abe_criar(8, TIPO_HASH_LINEAR);
    int      onde = -1;

    CASO("aberto: o túmulo é o que mantém a sondagem inteira");
    ASSERT_EQ(hash_abe_inserir(h, 8), OK);
    ASSERT_EQ(hash_abe_inserir(h, 16), OK);
    ASSERT_EQ(hash_abe_inserir(h, 24), OK);

    ASSERT_EQ(hash_abe_buscar(h, 8, &onde), OK);
    ASSERT_EQ(onde, 0);
    ASSERT_EQ(hash_abe_buscar(h, 16, &onde), OK);
    ASSERT_EQ(onde, 1);
    ASSERT_EQ(hash_abe_buscar(h, 24, &onde), OK);
    ASSERT_EQ(onde, 2);

    ASSERT_EQ(hash_abe_remover(h, 16), OK);
    ASSERT_EQ(hash_abe_tumulos(h), 1);
    ASSERT_TRUE(!hash_abe_contem(h, 16));

    /* A linha que separa a implementação certa da errada. */
    ASSERT_TRUE(hash_abe_contem(h, 24));
    ASSERT_EQ(hash_abe_buscar(h, 24, &onde), OK);
    ASSERT_EQ(onde, 2);

    CASO("aberto: a inserção reaproveita o túmulo");
    ASSERT_EQ(hash_abe_inserir(h, 32), OK);   /* também cai no balde 0 */
    ASSERT_EQ(hash_abe_tumulos(h), 0);
    ASSERT_EQ(hash_abe_buscar(h, 32, &onde), OK);
    ASSERT_EQ(onde, 1);                       /* o lugar que o 16 deixou */
    /* E não criou uma cópia do 24 nem o perdeu. */
    ASSERT_TRUE(hash_abe_contem(h, 24));
    ASSERT_EQ(hash_abe_tamanho(h), 3);

    CASO("aberto: reaproveitar o túmulo não duplica chave que está adiante");
    /* O 24 está na célula 2, depois de um túmulo em 1. Inserir 24 de novo tem
     * que achá-lo, e não parar no túmulo e criar uma segunda cópia. */
    ASSERT_EQ(hash_abe_remover(h, 32), OK);
    ASSERT_EQ(hash_abe_inserir(h, 24), OK);
    ASSERT_EQ(hash_abe_tamanho(h), 2);
    ASSERT_EQ(hash_abe_tumulos(h), 1);

    hash_abe_destruir(h);
}

static void suite_aberto(void)
{
    int s;

    for (s = 0; s < 3; s++) {
        HashAbe *h = hash_abe_criar(M_PRIMO, SONDAGENS[s]);
        int      onde = -1;
        int      i;

        CASO(NOME_SONDAGEM[s]);
        ASSERT_TRUE(h != NULL);
        ASSERT_EQ(hash_abe_baldes(h), M_PRIMO);
        ASSERT_EQ(hash_abe_tamanho(h), 0);
        ASSERT_EQ(hash_abe_buscar(h, 1, &onde), ERR_NAO_ENCONTRADO);
        ASSERT_EQ(hash_abe_remover(h, 1), ERR_NAO_ENCONTRADO);

        /* Metade da tabela: acima disso a quadrática pode legitimamente não
         * achar lugar, e isso é assunto do caso seguinte. */
        for (i = 0; i < M_PRIMO / 2; i++) {
            ASSERT_EQ(hash_abe_inserir(h, i * 3), OK);
        }
        ASSERT_EQ(hash_abe_tamanho(h), M_PRIMO / 2);

        for (i = 0; i < M_PRIMO / 2; i++) {
            ASSERT_TRUE(hash_abe_contem(h, i * 3));
            ASSERT_EQ(hash_abe_buscar(h, i * 3, &onde), OK);
            ASSERT_TRUE(onde >= 0 && onde < M_PRIMO);
        }

        /* Repetido não entra, em nenhuma das três. */
        ASSERT_EQ(hash_abe_inserir(h, 0), OK);
        ASSERT_EQ(hash_abe_tamanho(h), M_PRIMO / 2);

        /* Negativo não vira índice negativo. */
        ASSERT_EQ(hash_abe_inserir(h, -7), OK);
        ASSERT_TRUE(hash_abe_contem(h, -7));

        /* Remover tudo devolve a tabela vazia, e nada sobra encontrável. */
        for (i = 0; i < M_PRIMO / 2; i++) {
            ASSERT_EQ(hash_abe_remover(h, i * 3), OK);
            ASSERT_TRUE(!hash_abe_contem(h, i * 3));
        }
        ASSERT_EQ(hash_abe_remover(h, -7), OK);
        ASSERT_EQ(hash_abe_tamanho(h), 0);

        hash_abe_limpar(h);
        ASSERT_EQ(hash_abe_tumulos(h), 0);
        ASSERT_EQ(hash_abe_inserir(h, 5), OK);
        ASSERT_TRUE(hash_abe_contem(h, 5));

        hash_abe_destruir(h);
    }

    CASO("aberto: a tabela enche, e encher é erro e não travamento");
    {
        /* Linear com m primo visita a tabela inteira: ela só devolve ERR_CHEIA
         * quando de fato não sobrou célula. */
        HashAbe *h = hash_abe_criar(7, TIPO_HASH_LINEAR);
        int      i;

        for (i = 0; i < 7; i++) {
            ASSERT_EQ(hash_abe_inserir(h, i), OK);
        }
        ASSERT_EQ(hash_abe_tamanho(h), 7);
        ASSERT_EQ(hash_abe_inserir(h, 100), ERR_CHEIA);
        ASSERT_EQ(hash_abe_tamanho(h), 7);

        /* Abrir uma vaga por remoção faz a inserção voltar a caber. */
        ASSERT_EQ(hash_abe_remover(h, 3), OK);
        ASSERT_EQ(hash_abe_inserir(h, 100), OK);
        ASSERT_TRUE(hash_abe_contem(h, 100));

        hash_abe_destruir(h);
    }

    ASSERT_TRUE(hash_abe_criar(0, TIPO_HASH_LINEAR) == NULL);
    hash_abe_destruir(NULL);
}

/* O agrupamento, medido. É a razão de as três sondagens existirem, e sem um
 * número ele seria só uma palavra bonita.
 *
 * A carga é 0,94 de propósito, e isso É a lição: com a tabela pela metade as
 * três sondagens são indistinguíveis — medi, e com carga 0,79 a linear chega a
 * sondar MENOS que a dupla em algumas sementes. O agrupamento é um fenômeno de
 * tabela quase cheia, e afirmar a diferença numa carga baixa seria afirmar
 * ruído.
 *
 * Três sementes somadas, e não uma. Numa semente só, a diferença ainda é
 * sorteio; somadas, a linear sonda perto do dobro da dupla, e é isso que a
 * teoria prevê. */
static void suite_agrupamento(void)
{
    enum { M_GRANDE = 101, QUANTAS = 95 };

    static const unsigned int SEMENTES[3] = { 4242u, 7u, 20260827u };

    long sondas[3] = { 0, 0, 0 };
    int  s;
    int  k;

    CASO("agrupamento: com a tabela quase cheia, a linear sonda muito mais");

    for (s = 0; s < 3; s++) {
        for (k = 0; k < 3; k++) {
            HashAbe    *h = hash_abe_criar(M_GRANDE, SONDAGENS[s]);
            Aleatorio   rnd;
            const ev_t *evs;
            int32_t     n;
            int32_t     j;
            int         i;

            aleatorio_semear(&rnd, SEMENTES[k]);
            trace_reset();

            for (i = 0; i < QUANTAS; i++) {
                hash_abe_inserir(h, aleatorio_entre(&rnd, 0, 9999));
            }

            evs = trace_ptr();
            n = trace_len();
            for (j = 0; j < n; j++) {
                if (evs[j].kind == EV_COUNT && evs[j].a == CNT_SONDAGENS) {
                    sondas[s] += evs[j].b;
                }
            }

            /* As três chegam ao mesmo lugar: a diferença é o CAMINHO. */
            ASSERT_TRUE(hash_abe_tamanho(h) > QUANTAS - 5);
            hash_abe_destruir(h);
        }
    }

    ASSERT_TRUE(sondas[0] > 0 && sondas[1] > 0 && sondas[2] > 0);

    /* A ordem que a teoria prevê, com margem larga: o que se testa é que a
     * diferença EXISTE e tem o sinal certo, não um número. Medido, a linear
     * fica perto do dobro da dupla. */
    ASSERT_TRUE(sondas[0] > (sondas[2] * 5) / 4);
    /* A quadrática quebra os blocos contíguos, e fica no meio ou perto da
     * dupla — mas nunca tão ruim quanto a linear. */
    ASSERT_TRUE(sondas[0] > sondas[1]);

    trace_reset();
}

/* Fuzz contra um vetor de presença, nas quatro tabelas. */
static void suite_fuzz_hash(void)
{
    enum { OPERACOES = 4000 };

    Aleatorio rnd;
    int       presente[VALORES];
    int       s;
    int       i;

    CASO("fuzz: encadeado contra um vetor de presença");
    {
        HashEnc *h = hash_enc_criar(17);
        int      vivos = 0;
        int      divergencias = 0;
        int      removeu = 0;

        aleatorio_semear(&rnd, 90210u);
        for (i = 0; i < VALORES; i++) presente[i] = 0;
        trace_set_enabled(0);

        for (i = 0; i < OPERACOES; i++) {
            int    sorteio = aleatorio_entre(&rnd, 0, 99);
            elem_t v = aleatorio_entre(&rnd, 0, VALORES - 1);

            if (sorteio < 55) {
                if (hash_enc_inserir(h, v) != OK) divergencias++;
                if (!presente[v]) {
                    presente[v] = 1;
                    vivos++;
                }
            } else if (sorteio < 90) {
                int rc = hash_enc_remover(h, v);

                if ((rc == OK) != (presente[v] != 0)) divergencias++;
                if (rc == OK) {
                    removeu++;
                    presente[v] = 0;
                    vivos--;
                }
            } else {
                int balde = -1;
                int rc = hash_enc_buscar(h, v, &balde);

                if ((rc == OK) != (presente[v] != 0)) divergencias++;
            }

            if (hash_enc_tamanho(h) != vivos) divergencias++;
        }

        /* A invariante inteira, no fim: tudo que devia estar, está; e nada
         * mais que isso. */
        for (i = 0; i < VALORES; i++) {
            if (hash_enc_contem(h, i) != (presente[i] != 0)) divergencias++;
        }

        ASSERT_EQ(divergencias, 0);
        ASSERT_TRUE(removeu > 400);
        trace_set_enabled(1);
        hash_enc_destruir(h);
    }

    for (s = 0; s < 3; s++) {
        /* Tabela grande o bastante para não encher: encher é comportamento
         * legítimo, e misturá-lo ao fuzz esconderia divergência de verdade. */
        HashAbe *h = hash_abe_criar(509, SONDAGENS[s]);
        int      vivos = 0;
        int      divergencias = 0;
        int      removeu = 0;

        CASO(NOME_SONDAGEM[s]);
        aleatorio_semear(&rnd, 555u + (uint32_t) s);
        for (i = 0; i < VALORES; i++) presente[i] = 0;
        trace_set_enabled(0);

        for (i = 0; i < OPERACOES; i++) {
            int    sorteio = aleatorio_entre(&rnd, 0, 99);
            elem_t v = aleatorio_entre(&rnd, 0, VALORES - 1);

            if (sorteio < 55) {
                if (hash_abe_inserir(h, v) != OK) divergencias++;
                if (!presente[v]) {
                    presente[v] = 1;
                    vivos++;
                }
            } else if (sorteio < 90) {
                int rc = hash_abe_remover(h, v);

                if ((rc == OK) != (presente[v] != 0)) divergencias++;
                if (rc == OK) {
                    removeu++;
                    presente[v] = 0;
                    vivos--;
                }
            } else {
                int onde = -1;
                int rc = hash_abe_buscar(h, v, &onde);

                if ((rc == OK) != (presente[v] != 0)) divergencias++;
            }

            if (hash_abe_tamanho(h) != vivos) divergencias++;
            /* A cada operação: é o único jeito de a falha apontar para a
             * operação que a causou, e a remoção com túmulo é justamente o
             * lugar em que um erro fica escondido por muito tempo. */
            if (i % 50 == 0) {
                int k;

                for (k = 0; k < VALORES; k++) {
                    if (hash_abe_contem(h, k) != (presente[k] != 0)) {
                        divergencias++;
                        break;
                    }
                }
            }
        }

        for (i = 0; i < VALORES; i++) {
            if (hash_abe_contem(h, i) != (presente[i] != 0)) divergencias++;
        }

        ASSERT_EQ(divergencias, 0);
        ASSERT_TRUE(removeu > 400);
        trace_set_enabled(1);
        hash_abe_destruir(h);
    }
}

static void suite_vtable_hash(void)
{
    CASO("o vtable das tabelas hash");
    /* Numa tabela hash não existe "o primeiro" nem "o menor": os dois membros
     * sem argumento ficam nulos, e api.c responde por isso. */
    ASSERT_TRUE(HASH_ENC.remover == NULL);
    ASSERT_TRUE(HASH_ENC.consultar == NULL);
    ASSERT_TRUE(HASH_LINEAR.remover == NULL);
    ASSERT_TRUE(HASH_LINEAR.consultar == NULL);

    ASSERT_TRUE(HASH_ENC.buscar != NULL);
    ASSERT_TRUE(HASH_ENC.remover_valor != NULL);
    ASSERT_TRUE(HASH_ENC.percurso == NULL);

    /* As três abertas são o mesmo código; o que muda é o `criar`, que decide a
     * sondagem guardada na estrutura. */
    ASSERT_TRUE(HASH_LINEAR.criar != HASH_QUAD.criar);
    ASSERT_TRUE(HASH_QUAD.criar != HASH_DUPLO.criar);
    ASSERT_TRUE(HASH_LINEAR.inserir == HASH_QUAD.inserir);
    ASSERT_TRUE(HASH_QUAD.buscar == HASH_DUPLO.buscar);
}

void suite_hash(void)
{
    suite_vtable_hash();
    suite_encadeado();
    suite_tumulo();
    suite_aberto();
    suite_agrupamento();
    suite_fuzz_hash();
}
