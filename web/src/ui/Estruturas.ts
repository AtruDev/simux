/* O catálogo de estruturas da aba, e o que muda de uma para outra.
 *
 * Pilha e fila compartilham as mesmas quatro operações do vtable — inserir,
 * remover, consultar, limpar. O que muda é o nome de cada uma e o
 * renderizador. Manter isto numa tabela evita um `if (é fila)` espalhado pela
 * interface. */

import { Tipo } from "../core/ops";
import type { Chave } from "../i18n";

export type Mundo = "encadeada" | "vetor";

export interface Estrutura {
  tipo: number;
  nome: Chave;
  mundo: Mundo;
  /* Famílias iguais aparecem juntas no seletor de implementação. */
  familia: "pilha" | "fila";
  rotuloInserir: Chave;
  rotuloRemover: Chave;
  rotuloConsultar: Chave;
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
];

export function estruturaDe(tipo: number): Estrutura {
  return ESTRUTURAS.find((e) => e.tipo === tipo) ?? ESTRUTURAS[0]!;
}
