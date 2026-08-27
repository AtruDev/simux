/* core/disco/acessos.h — as duas macros que contam e anunciam um acesso.
 *
 * São macros pelo mesmo motivo das de core/sort/passos.h: `__LINE__` e
 * `TR_SRC` expandem no ponto de chamada, então o painel de código destaca a
 * linha do algoritmo que leu a página. Uma função faria o destaque cair
 * dentro do paginador, que é o lugar onde não há nada para aprender.
 *
 * Contar e anunciar na mesma macro é o que impede o número do painel de
 * divergir do desenho: os dois saem da mesma linha. */

#ifndef DS_DISCO_ACESSOS_H
#define DS_DISCO_ACESSOS_H

#include "ds/ids.h"
#include "ds/paginador.h"
#include "ds/trace.h"

#define LER_PAGINA(pag, pagina) do {                            \
    int pagina_ = (int) (pagina);                               \
    paginador_leu(pag);                                         \
    TR(EV_DISK_READ, .a = pagina_);                             \
    TR(EV_COUNT, .a = CNT_DISCO_LE, .b = +1);                   \
} while (0)

#define ESCREVER_PAGINA(pag, pagina) do {                       \
    int pagina_ = (int) (pagina);                               \
    paginador_escreveu(pag);                                    \
    TR(EV_DISK_WRITE, .a = pagina_);                            \
    TR(EV_COUNT, .a = CNT_DISCO_ESCREVE, .b = +1);              \
} while (0)

#endif /* DS_DISCO_ACESSOS_H */
