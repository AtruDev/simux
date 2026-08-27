/* O catálogo de estruturas da aba, e o que muda de uma para outra.
 *
 * Pilha e fila compartilham as mesmas quatro operações do vtable — inserir,
 * remover, consultar, limpar. O que muda é o nome de cada uma e o
 * renderizador. Manter isto numa tabela evita um `if (é fila)` espalhado pela
 * interface.
 *
 * A lista acrescentou uma terceira coluna a essa tabela: `posicoes`. Quem tem
 * posição ganha o campo e os botões de inserir/remover em posição e de buscar;
 * quem não tem, nem vê que eles existem. */

import { Tipo } from "../core/ops";
import type { Chave } from "../i18n";

/* Qual renderizador e quais métricas a tela usa.
 *
 * "ordenacao" não tem entrada em ESTRUTURAS: a aba 2 não escolhe estrutura,
 * escolhe algoritmo. Ela está aqui porque o painel de métricas e o log são os
 * mesmos das duas abas, e é o mundo que diz a eles como ler o modelo. */
export type Mundo =
  | "encadeada"
  | "vetor"
  | "lista"
  | "ordenacao"
  | "busca"
  | "arvore"
  | "hashEnc"
  | "hashAbe";

export interface Estrutura {
  tipo: number;
  nome: Chave;
  mundo: Mundo;
  /* Famílias iguais aparecem juntas no seletor de implementação. */
  familia: "pilha" | "fila" | "lista" | "busca" | "arvore" | "hash";
  rotuloInserir: Chave;
  rotuloRemover: Chave;
  rotuloConsultar: Chave;
  /* A posição é argumento das operações desta estrutura. */
  posicoes?: boolean;
  /* O vtable dela tem `buscar`. Não é o mesmo que ter posição: no vetor
   * ordenado a posição é consequência do valor, e só a busca existe. */
  buscavel?: boolean;
  /* Remove por valor e percorre — os dois membros que a árvore trouxe para o
   * vtable. Numa árvore a posição não existe, e é a remoção por valor que traz
   * os três casos. */
  arvore?: boolean;
  /* Remove por valor, mas SEM percurso: a tabela hash não tem ordem para
   * percorrer. */
  porValor?: boolean;
  /* Não tem operação sem argumento. Numa tabela hash não existe "o primeiro"
   * nem "o menor" — a ordem dos elementos é acidente da função hash, e os dois
   * ponteiros ficam nulos no vtable. A interface esconde os botões em vez de
   * mostrá-los e recusar. */
  semExtremos?: boolean;
}

export const ESTRUTURAS: Estrutura[] = [
  {
    tipo: Tipo.TIPO_PILHA_ENC,
    nome: "estrutura.pilhaEnc",
    mundo: "encadeada",
    familia: "pilha",
    rotuloInserir: "op.push",
    rotuloRemover: "op.pop",
    rotuloConsultar: "op.topo",
  },
  {
    tipo: Tipo.TIPO_PILHA_VET,
    nome: "estrutura.pilhaVet",
    mundo: "vetor",
    familia: "pilha",
    rotuloInserir: "op.push",
    rotuloRemover: "op.pop",
    rotuloConsultar: "op.topo",
  },
  {
    tipo: Tipo.TIPO_FILA_ENC,
    nome: "estrutura.filaEnc",
    mundo: "encadeada",
    familia: "fila",
    rotuloInserir: "op.enfileirar",
    rotuloRemover: "op.desenfileirar",
    rotuloConsultar: "op.frente",
  },
  {
    tipo: Tipo.TIPO_FILA_VET,
    nome: "estrutura.filaVet",
    mundo: "vetor",
    familia: "fila",
    rotuloInserir: "op.enfileirar",
    rotuloRemover: "op.desenfileirar",
    rotuloConsultar: "op.frente",
  },
  {
    tipo: Tipo.TIPO_LISTA_SIMPLES,
    nome: "estrutura.listaSimples",
    mundo: "lista",
    familia: "lista",
    rotuloInserir: "op.inserirInicio",
    rotuloRemover: "op.removerInicio",
    rotuloConsultar: "op.primeiro",
    posicoes: true,
    buscavel: true,
  },
  {
    tipo: Tipo.TIPO_LISTA_DUPLA,
    nome: "estrutura.listaDupla",
    mundo: "lista",
    familia: "lista",
    rotuloInserir: "op.inserirInicio",
    rotuloRemover: "op.removerInicio",
    rotuloConsultar: "op.primeiro",
    posicoes: true,
    buscavel: true,
  },
  {
    tipo: Tipo.TIPO_LISTA_CIRCULAR,
    nome: "estrutura.listaCircular",
    mundo: "lista",
    familia: "lista",
    rotuloInserir: "op.inserirInicio",
    rotuloRemover: "op.removerInicio",
    rotuloConsultar: "op.primeiro",
    posicoes: true,
    buscavel: true,
  },
  /* As duas buscas são o caso mais puro do argumento do projeto: um TAD, duas
   * implementações, e a diferença entre elas é UMA função. Todo o resto — o
   * vetor ordenado, a inserção que desloca, a remoção — é o mesmo código, e é
   * isso que faz o contador de comparações medir só o que interessa.
   *
   * O modo comparar não precisou de nada novo para elas. */
  {
    tipo: Tipo.TIPO_BUSCA_SEQ,
    nome: "estrutura.buscaSeq",
    mundo: "busca",
    familia: "busca",
    rotuloInserir: "op.inserirOrdenado",
    rotuloRemover: "op.removerMenor",
    rotuloConsultar: "op.menor",
    buscavel: true,
  },
  {
    tipo: Tipo.TIPO_BUSCA_BIN,
    nome: "estrutura.buscaBin",
    mundo: "busca",
    familia: "busca",
    rotuloInserir: "op.inserirOrdenado",
    rotuloRemover: "op.removerMenor",
    rotuloConsultar: "op.menor",
    buscavel: true,
  },
  /* A ABB é a primeira estrutura que não é linear, e mesmo assim entrou pelo
   * mesmo vtable: `remover` sem argumento quer dizer "remova o menor", que é
   * o que uma fila de prioridade faria, e os dois membros novos cobrem o que
   * ela tem de próprio. */
  {
    tipo: Tipo.TIPO_ABB,
    nome: "estrutura.abb",
    mundo: "arvore",
    familia: "arvore",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerMenor",
    rotuloConsultar: "op.menor",
    buscavel: true,
    arvore: true,
  },
  /* Mesma família da ABB, e é isso que faz o modo comparar existir para elas:
   * a mesma sequência crescente nas duas, lado a lado, é o argumento inteiro
   * de a AVL existir — e não precisa de uma palavra de texto. */
  {
    tipo: Tipo.TIPO_AVL,
    nome: "estrutura.avl",
    mundo: "arvore",
    familia: "arvore",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerMenor",
    rotuloConsultar: "op.menor",
    buscavel: true,
    arvore: true,
  },
  /* As quatro tabelas hash, todas na mesma família: o modo comparar as põe
   * lado a lado, e é ali que o agrupamento das três sondagens abertas fica
   * visível de uma vez. */
  {
    tipo: Tipo.TIPO_HASH_ENC,
    nome: "estrutura.hashEnc",
    mundo: "hashEnc",
    familia: "hash",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerValor",
    rotuloConsultar: "op.buscar",
    buscavel: true,
    porValor: true,
    semExtremos: true,
  },
  {
    tipo: Tipo.TIPO_HASH_LINEAR,
    nome: "estrutura.hashLinear",
    mundo: "hashAbe",
    familia: "hash",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerValor",
    rotuloConsultar: "op.buscar",
    buscavel: true,
    porValor: true,
    semExtremos: true,
  },
  {
    tipo: Tipo.TIPO_HASH_QUAD,
    nome: "estrutura.hashQuad",
    mundo: "hashAbe",
    familia: "hash",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerValor",
    rotuloConsultar: "op.buscar",
    buscavel: true,
    porValor: true,
    semExtremos: true,
  },
  {
    tipo: Tipo.TIPO_HASH_DUPLO,
    nome: "estrutura.hashDuplo",
    mundo: "hashAbe",
    familia: "hash",
    rotuloInserir: "op.inserir",
    rotuloRemover: "op.removerValor",
    rotuloConsultar: "op.buscar",
    buscavel: true,
    porValor: true,
    semExtremos: true,
  },
];

export function estruturaDe(tipo: number): Estrutura {
  return ESTRUTURAS.find((e) => e.tipo === tipo) ?? ESTRUTURAS[0]!;
}

/** Todas as implementações de uma família, na ordem em que aparecem no menu.
 *
 * É o conjunto que o modo comparar põe lado a lado. São duas para pilha e
 * fila, e três para lista — o Player já aceita quantas trilhas forem. A ordem
 * é a do catálogo, para a faixa de cima não trocar de lugar ao mudar de
 * família. */
export function parDaFamilia(familia: Estrutura["familia"]): Estrutura[] {
  return ESTRUTURAS.filter((e) => e.familia === familia);
}
