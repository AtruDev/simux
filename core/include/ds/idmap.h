/* core/include/ds/idmap.h — ponteiro do C para id estável no frontend.
 *
 * O JS não pode ver ponteiros, então cada nó precisa de um inteiro. Usar o
 * próprio endereço não serve: malloc recicla endereços depois do free, e um
 * nó novo herdaria a identidade visual de um nó morto — a animação faria
 * coisas absurdas, movendo o nó errado.
 *
 * A tabela resolve isso porque id_esquece() é chamado no free: o endereço
 * volta a ser desconhecido, e o próximo id_de() sobre ele cria um id inédito.
 *
 * A vantagem sobre pôr um campo `int id` dentro do nó é que as struct das
 * estruturas ficam idênticas às da matéria, sem campo de visualização. */

#ifndef DS_IDMAP_H
#define DS_IDMAP_H

#include <stdint.h>

/* Id do ponteiro, criando um se for a primeira vez. id_de(NULL) devolve 0,
 * e 0 significa NULL no frontend — é isso que simplifica EV_EDGE_SET. */
int32_t id_de(const void *p);

/* Esquece o ponteiro. Chame junto do free, sempre. */
void    id_esquece(const void *p);

/* Zera a tabela e volta a numerar do 1. Entre sessões e entre testes. */
void    idmap_reset(void);

/* Quantos ponteiros a tabela conhece. Serve de verificação de vazamento:
 * depois de limpar a estrutura, tem que ser zero. */
int32_t idmap_vivos(void);

/* Verdadeiro se em algum momento faltou memória para registrar um ponteiro.
 * Nesse caso o id devolvido foi único, mas não estável. */
int     idmap_falhou(void);

#endif /* DS_IDMAP_H */
