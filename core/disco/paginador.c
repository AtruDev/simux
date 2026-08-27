/* core/disco/paginador.c — dez linhas, e o número que a Fase 5 inteira gira
 * em torno.
 *
 * Sem trace aqui de propósito: quem emite EV_DISK_READ é a macro de
 * acessos.h, expandida no ponto de chamada, para o painel de código destacar
 * a linha do algoritmo que leu a página — e não uma linha deste arquivo, que
 * não ensina nada. */

#include "ds/paginador.h"

void paginador_iniciar(Paginador *p)
{
    p->proxima = 1;     /* a página 0 é "nenhuma" */
    p->leituras = 0;
    p->escritas = 0;
}

int paginador_alocar(Paginador *p)
{
    return p->proxima++;
}

void paginador_leu(Paginador *p)
{
    p->leituras++;
}

void paginador_escreveu(Paginador *p)
{
    p->escritas++;
}

long paginador_leituras(const Paginador *p)
{
    return p->leituras;
}

long paginador_escritas(const Paginador *p)
{
    return p->escritas;
}
