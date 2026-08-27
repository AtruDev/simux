/* core/include/ds/hash.h — as duas famílias de tabela hash.
 *
 * A função é a mesma nas duas, e é a da matéria: h(k) = k mod m. Não é a
 * melhor função hash que existe, e é a certa aqui — ela torna a colisão
 * previsível, e uma colisão que dá para prever é uma colisão que dá para
 * ensinar. Com m = 8, inserir 8, 16 e 24 colide três vezes no balde 0, de
 * propósito.
 *
 * O `m` é escolhido por quem usa, e é a segunda metade da lição: com m primo
 * as colisões se espalham, com m potência de dois elas se acumulam nos mesmos
 * baldes. Trocar 8 por 7 muda o desenho inteiro sem trocar uma linha de código.
 *
 * O que muda entre as duas famílias é o que fazer QUANDO colide:
 *
 *   encadeada    o balde vira uma lista, e o novo entra nela. Nunca enche;
 *                o custo é um malloc por elemento e a cadeia crescendo.
 *   aberta       procura outro balde livre no próprio arranjo. Não aloca
 *                nada; em troca, enche — e o agrupamento faz a busca piorar
 *                muito antes disso.
 *
 * A aberta tem três implementações que diferem em UMA função: por quanto andar
 * quando o balde está ocupado. É o mesmo arranjo das duas buscas do vetor
 * ordenado, e pelo mesmo motivo — lado a lado, o que muda é o agrupamento. */

#ifndef DS_HASH_H
#define DS_HASH_H

#include "ds/tipos.h"

/* ---- hash encadeado ----------------------------------------------------- */

typedef struct HashEnc HashEnc;

HashEnc *hash_enc_criar(int m);
void     hash_enc_destruir(HashEnc *h);

int      hash_enc_inserir(HashEnc *h, elem_t valor);
int      hash_enc_remover(HashEnc *h, elem_t valor);
/* Devolve OK e o balde por ponteiro, ou ERR_NAO_ENCONTRADO. */
int      hash_enc_buscar(const HashEnc *h, elem_t valor, int *balde);
void     hash_enc_limpar(HashEnc *h);

int      hash_enc_tamanho(const HashEnc *h);
int      hash_enc_baldes(const HashEnc *h);
/* O maior número de elementos num balde. É a medida de qualidade da função
 * hash: numa tabela bem espalhada ele fica perto de n/m. */
int      hash_enc_maior_cadeia(const HashEnc *h);

/* ---- hash aberto -------------------------------------------------------- */

typedef struct HashAbe HashAbe;

/* `sondagem` é um TIPO_HASH_* de ids.h: linear, quadrática ou dupla. */
HashAbe *hash_abe_criar(int m, int sondagem);
void     hash_abe_destruir(HashAbe *h);

int      hash_abe_inserir(HashAbe *h, elem_t valor);
int      hash_abe_remover(HashAbe *h, elem_t valor);
int      hash_abe_buscar(const HashAbe *h, elem_t valor, int *posicao);
void     hash_abe_limpar(HashAbe *h);

int      hash_abe_tamanho(const HashAbe *h);
int      hash_abe_baldes(const HashAbe *h);

/* Quantas células estão marcadas como túmulo — removidas, mas que a sondagem
 * ainda tem que atravessar. É a dívida que a remoção deixa, e o número que
 * explica por que uma tabela aberta piora com o uso mesmo sem crescer. */
int      hash_abe_tumulos(const HashAbe *h);

/* ---- invariantes, para os testes. Não instrumentam. -------------------- */

/* Tudo o que foi inserido é encontrado, e nada removido é encontrado. É a
 * invariante que o plano pede, e a única que uma tabela hash tem. */
int      hash_enc_contem(const HashEnc *h, elem_t valor);
int      hash_abe_contem(const HashAbe *h, elem_t valor);

#endif /* DS_HASH_H */
