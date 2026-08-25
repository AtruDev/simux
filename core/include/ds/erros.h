/* core/include/ds/erros.h — códigos de status das funções públicas.
 *
 * Toda função pública do core devolve um destes; a saída de verdade sai
 * por ponteiro. O frontend traduz o código via i18n — o C nunca devolve
 * texto.
 *
 * Lido por tools/gen_enums.py junto com ids.h. */

#ifndef DS_ERROS_H
#define DS_ERROS_H

typedef enum {
    OK,                     /* nada de errado                              */
    ERR_SEM_MEMORIA,        /* malloc devolveu NULL                        */
    ERR_VAZIA,              /* remover/consultar estrutura sem elementos   */
    ERR_CHEIA,              /* inserir em estrutura de capacidade fixa     */
    ERR_NAO_ENCONTRADO,     /* busca sem resultado                         */
    ERR_ARG_INVALIDO,       /* argumento fora do domínio                   */
    ERR_OP_DESCONHECIDA,    /* ds_call com op que não existe               */
    ERR_SEM_SESSAO,         /* ds_call antes de ds_sessao_nova             */
    ERR_COUNT
} ds_status;

#endif /* DS_ERROS_H */
