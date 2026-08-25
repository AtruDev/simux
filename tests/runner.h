/* tests/runner.h — o mínimo para escrever testes sem framework.
 *
 * Registro por construtor exigiria extensão de compilador, então as suítes
 * são declaradas aqui e chamadas na mão pelo runner. Acrescentar uma suíte
 * é acrescentar uma linha em cada um dos dois lugares. */

#ifndef RUNNER_H
#define RUNNER_H

void caso(const char *nome);
void verificar_eq(const char *arquivo, int linha, const char *expr,
                  long long obtido, long long esperado);
void verificar_true(const char *arquivo, int linha, const char *expr, int ok);

#define CASO(nome)      caso(nome)
#define ASSERT_EQ(obtido, esperado)                                  \
    verificar_eq(__FILE__, __LINE__, #obtido,                        \
                 (long long) (obtido), (long long) (esperado))
#define ASSERT_TRUE(expr)                                            \
    verificar_true(__FILE__, __LINE__, #expr, (expr) != 0)

/* suítes */
void suite_trace(void);
void suite_idmap(void);

#endif /* RUNNER_H */
