/* core/include/ds/paginador.h — o disco simulado, e a única métrica que
 * importa em memória secundária.
 *
 * Um acesso a disco custa uns dez milhões de vezes mais que um acesso à RAM.
 * Essa razão é o motivo de a árvore B existir, e é por isso que ela é larga e
 * baixa em vez de estreita e alta: cada nível a menos é um acesso a menos, e
 * comparar mais chaves dentro de uma página que já foi lida é de graça perto
 * de ler outra página.
 *
 * Esta é a estrutura mais simples do projeto, e é de propósito. Ela não guarda
 * dado nenhum — o dado continua na memória do processo, e é assim que ele
 * pode ser desenhado. O que ela faz é numerar páginas e CONTAR: quantas foram
 * lidas, quantas foram escritas. Simular o conteúdo do disco não tornaria
 * nenhum número mais verdadeiro, e tornaria o código muito menos legível.
 *
 * Não há cache aqui, e isso é uma escolha. Um buffer pool deixaria a raiz
 * praticamente sempre na memória, e os números seriam mais realistas e menos
 * ensináveis: a comparação entre uma árvore de 20 níveis e uma de 3 fica
 * bem mais clara quando cada nível visitado custa exatamente um acesso. */

#ifndef DS_PAGINADOR_H
#define DS_PAGINADOR_H

typedef struct {
    int  proxima;       /* número da próxima página a entregar */
    long leituras;
    long escritas;
} Paginador;

void paginador_iniciar(Paginador *p);

/* Entrega o número de uma página nova. A primeira é a 1: a página 0 fica
 * reservada para "nenhuma", pelo mesmo motivo que o id 0 é NULL no idmap. */
int  paginador_alocar(Paginador *p);

/* Contam o acesso. Não emitem evento: quem emite é a macro de
 * core/disco/acessos.h, para o `__LINE__` cair no algoritmo e não aqui. */
void paginador_leu(Paginador *p);
void paginador_escreveu(Paginador *p);

long paginador_leituras(const Paginador *p);
long paginador_escritas(const Paginador *p);

#endif /* DS_PAGINADOR_H */
