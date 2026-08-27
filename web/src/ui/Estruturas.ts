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

export type Mundo = "encadeada" | "vetor" | "lista";

export interface Estrutura {
  tipo: number;
  nome: Chave;
  mundo: Mundo;
  /* Famílias iguais aparecem juntas no seletor de implementação. */
  familia: "pilha" | "fila" | "lista";
  rotuloInserir: Chave;
  rotuloRemover: Chave;
  rotuloConsultar: Chave;
  /* A posição é argumento das operações desta estrutura. */
  posicoes?: boolean;
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
