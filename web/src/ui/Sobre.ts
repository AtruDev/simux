/* O que a aba tem a dizer sobre cada estrutura além de desenhá-la.
 *
 * Três coisas, e nenhuma delas é a definição — quem chegou aqui já viu a
 * definição no slide:
 *
 *   CUSTOS   a tabela de complexidade, com médio e pior caso em colunas
 *            separadas. Separar é o ponto: numa tabela hash os dois números
 *            são O(1) e O(n), e é essa distância que a aula sobre função hash
 *            existe para explicar. Juntar tudo numa coluna só esconderia
 *            justamente o que interessa.
 *
 *   QUANDO   para que serve, em uma frase. Não "uma pilha é LIFO", e sim onde
 *            alguém usaria uma.
 *
 *   PEGA     o preço, o erro clássico, ou a razão de a estrutura seguinte
 *            existir. É a parte que um slide costuma não ter, e é a que faz o
 *            par encadeada/vetor valer duas telas em vez de uma.
 *
 * As complexidades não passam pelo i18n: `O(log n)` é `O(log n)` nos dois
 * idiomas. Os rótulos das operações também não são escritos aqui — eles vêm
 * do próprio catálogo (`rotuloInserir` e companhia), então uma pilha diz
 * "empilhar" e uma fila diz "enfileirar" sem esta tabela saber disso. */

import { Tipo } from "../core/ops";
import type { Chave } from "../i18n";

/** Qual operação, quanto custa no caso médio, quanto custa no pior. */
export type Custo = readonly [operacao: Chave, medio: string, pior: string];

export interface Sobre {
  custos: readonly Custo[];
  quando: Chave;
  pega: Chave;
}

/* Notação usada nas tabelas:
 *   n  elementos na estrutura
 *   m  baldes da tabela hash
 *   t  grau mínimo da árvore B — cada página guarda até 2t-1 chaves          */

export const SOBRE: Partial<Record<number, Sobre>> = {
  [Tipo.TIPO_PILHA_ENC]: {
    custos: [
      ["op.push", "O(1)", "O(1)"],
      ["op.pop", "O(1)", "O(1)"],
      ["op.topo", "O(1)", "O(1)"],
    ],
    quando: "sobre.pilhaEnc.quando",
    pega: "sobre.pilhaEnc.pega",
  },
  [Tipo.TIPO_PILHA_VET]: {
    custos: [
      ["op.push", "O(1)", "O(1)"],
      ["op.pop", "O(1)", "O(1)"],
      ["op.topo", "O(1)", "O(1)"],
    ],
    quando: "sobre.pilhaVet.quando",
    pega: "sobre.pilhaVet.pega",
  },
  [Tipo.TIPO_FILA_ENC]: {
    custos: [
      ["op.enfileirar", "O(1)", "O(1)"],
      ["op.desenfileirar", "O(1)", "O(1)"],
      ["op.frente", "O(1)", "O(1)"],
    ],
    quando: "sobre.filaEnc.quando",
    pega: "sobre.filaEnc.pega",
  },
  [Tipo.TIPO_FILA_VET]: {
    custos: [
      ["op.enfileirar", "O(1)", "O(1)"],
      ["op.desenfileirar", "O(1)", "O(1)"],
      ["op.frente", "O(1)", "O(1)"],
    ],
    quando: "sobre.filaVet.quando",
    pega: "sobre.filaVet.pega",
  },
  [Tipo.TIPO_LISTA_SIMPLES]: {
    custos: [
      ["op.inserirInicio", "O(1)", "O(1)"],
      ["op.inserirEm", "O(n)", "O(n)"],
      ["op.buscar", "O(n)", "O(n)"],
    ],
    quando: "sobre.listaSimples.quando",
    pega: "sobre.listaSimples.pega",
  },
  [Tipo.TIPO_LISTA_DUPLA]: {
    custos: [
      ["op.inserirInicio", "O(1)", "O(1)"],
      ["op.inserirEm", "O(n)", "O(n)"],
      ["op.buscar", "O(n)", "O(n)"],
    ],
    quando: "sobre.listaDupla.quando",
    pega: "sobre.listaDupla.pega",
  },
  [Tipo.TIPO_LISTA_CIRCULAR]: {
    custos: [
      ["op.inserirInicio", "O(1)", "O(1)"],
      ["op.inserirEm", "O(n)", "O(n)"],
      ["op.buscar", "O(n)", "O(n)"],
    ],
    quando: "sobre.listaCircular.quando",
    pega: "sobre.listaCircular.pega",
  },
  [Tipo.TIPO_BUSCA_SEQ]: {
    custos: [
      ["op.buscar", "O(n)", "O(n)"],
      ["op.inserir", "O(n)", "O(n)"],
    ],
    quando: "sobre.buscaSeq.quando",
    pega: "sobre.buscaSeq.pega",
  },
  [Tipo.TIPO_BUSCA_BIN]: {
    custos: [
      ["op.buscar", "O(log n)", "O(log n)"],
      ["op.inserir", "O(n)", "O(n)"],
    ],
    quando: "sobre.buscaBin.quando",
    pega: "sobre.buscaBin.pega",
  },
  [Tipo.TIPO_ABB]: {
    custos: [
      ["op.inserir", "O(log n)", "O(n)"],
      ["op.buscar", "O(log n)", "O(n)"],
      ["op.removerValor", "O(log n)", "O(n)"],
    ],
    quando: "sobre.abb.quando",
    pega: "sobre.abb.pega",
  },
  [Tipo.TIPO_AVL]: {
    custos: [
      ["op.inserir", "O(log n)", "O(log n)"],
      ["op.buscar", "O(log n)", "O(log n)"],
      ["op.removerValor", "O(log n)", "O(log n)"],
    ],
    quando: "sobre.avl.quando",
    pega: "sobre.avl.pega",
  },
  [Tipo.TIPO_HASH_ENC]: {
    custos: [
      ["op.inserir", "O(1)", "O(n)"],
      ["op.buscar", "O(1)", "O(n)"],
      ["op.removerValor", "O(1)", "O(n)"],
    ],
    quando: "sobre.hashEnc.quando",
    pega: "sobre.hashEnc.pega",
  },
  [Tipo.TIPO_HASH_LINEAR]: {
    custos: [
      ["op.inserir", "O(1)", "O(n)"],
      ["op.buscar", "O(1)", "O(n)"],
      ["op.removerValor", "O(1)", "O(n)"],
    ],
    quando: "sobre.hashLinear.quando",
    pega: "sobre.hashLinear.pega",
  },
  [Tipo.TIPO_HASH_QUAD]: {
    custos: [
      ["op.inserir", "O(1)", "O(n)"],
      ["op.buscar", "O(1)", "O(n)"],
      ["op.removerValor", "O(1)", "O(n)"],
    ],
    quando: "sobre.hashQuad.quando",
    pega: "sobre.hashQuad.pega",
  },
  [Tipo.TIPO_HASH_DUPLO]: {
    custos: [
      ["op.inserir", "O(1)", "O(n)"],
      ["op.buscar", "O(1)", "O(n)"],
      ["op.removerValor", "O(1)", "O(n)"],
    ],
    quando: "sobre.hashDuplo.quando",
    pega: "sobre.hashDuplo.pega",
  },
  /* Nas duas de disco a coluna que importa não é comparação, é PÁGINA — e é
   * por isso que a complexidade delas é escrita na base t. */
  [Tipo.TIPO_ARVORE_B]: {
    custos: [
      ["op.inserir", "O(log_t n)", "O(log_t n)"],
      ["op.buscar", "O(log_t n)", "O(log_t n)"],
      ["op.varrer", "O(n/t)", "O(n/t)"],
    ],
    quando: "sobre.arvoreB.quando",
    pega: "sobre.arvoreB.pega",
  },
  [Tipo.TIPO_ARVORE_B_MAIS]: {
    custos: [
      ["op.inserir", "O(log_t n)", "O(log_t n)"],
      ["op.buscar", "O(log_t n)", "O(log_t n)"],
      ["op.varrer", "O(n/t)", "O(n/t)"],
    ],
    quando: "sobre.arvoreBMais.quando",
    pega: "sobre.arvoreBMais.pega",
  },
};

export function sobreDe(tipo: number): Sobre | undefined {
  return SOBRE[tipo];
}
