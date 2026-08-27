/* tests/runner.c — runner próprio. Sem dependência, que é a regra do core. */

#include <stdio.h>

#include "runner.h"

static const char *g_caso = "(sem nome)";
static int         g_checagens;
static int         g_falhas;

void caso(const char *nome)
{
    g_caso = nome;
}

static void relatar(const char *arquivo, int linha, const char *expr)
{
    printf("FALHOU  %s\n        %s:%d: %s\n", g_caso, arquivo, linha, expr);
}

void verificar_eq(const char *arquivo, int linha, const char *expr,
                  long long obtido, long long esperado)
{
    g_checagens++;
    if (obtido == esperado) {
        return;
    }
    g_falhas++;
    relatar(arquivo, linha, expr);
    printf("        obtido %lld, esperado %lld\n", obtido, esperado);
}

void verificar_true(const char *arquivo, int linha, const char *expr, int ok)
{
    g_checagens++;
    if (ok) {
        return;
    }
    g_falhas++;
    relatar(arquivo, linha, expr);
}

int main(void)
{
    suite_trace();
    suite_idmap();
    suite_pilha();
    suite_fila();
    suite_fuzz();
    suite_lista();
    suite_ordenacao();
    suite_busca();
    suite_arvore();
    suite_hash();
    suite_api();

    printf("\n%d checagens, %d falha(s)\n", g_checagens, g_falhas);
    return g_falhas != 0;
}
