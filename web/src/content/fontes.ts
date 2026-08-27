/* O painel de código mostra o .c de verdade, importado cru.
 *
 * Não é pseudocódigo reescrito para a tela: é o mesmo arquivo que compilou
 * para o wasm que acabou de rodar. É por isso que a linha destacada pode ser
 * confiada — ela vem do __LINE__ que a macro TR gravou. */

import fonteAbb from "../../../core/arvore/abb.c?raw";
import fonteApi from "../../../core/api/api.c?raw";
import fonteVetorOrd from "../../../core/busca/vetor_ord.c?raw";
import fonteBolha from "../../../core/sort/bolha.c?raw";
import fonteCena from "../../../core/sort/cena.c?raw";
import fonteFilaEnc from "../../../core/ds/fila_enc.c?raw";
import fonteFilaVet from "../../../core/ds/fila_vet.c?raw";
import fonteListaCircular from "../../../core/ds/lista_circular.c?raw";
import fonteListaDupla from "../../../core/ds/lista_dupla.c?raw";
import fonteListaSimples from "../../../core/ds/lista_simples.c?raw";
import fontePilhaEnc from "../../../core/ds/pilha_enc.c?raw";
import fonteInsercao from "../../../core/sort/insercao.c?raw";
import fonteMerge from "../../../core/sort/merge.c?raw";
import fontePilhaVet from "../../../core/ds/pilha_vet.c?raw";
import fonteQuick from "../../../core/sort/quick.c?raw";
import fonteSelecao from "../../../core/sort/selecao.c?raw";
import fonteShell from "../../../core/sort/shell.c?raw";
import { Src } from "../core/ops";

export interface Fonte {
  arquivo: string;
  linhas: string[];
}

const bruto: Partial<Record<number, { arquivo: string; texto: string }>> = {
  [Src.SRC_API]: { arquivo: "core/api/api.c", texto: fonteApi },
  [Src.SRC_PILHA_ENC]: { arquivo: "core/ds/pilha_enc.c", texto: fontePilhaEnc },
  [Src.SRC_PILHA_VET]: { arquivo: "core/ds/pilha_vet.c", texto: fontePilhaVet },
  [Src.SRC_FILA_ENC]: { arquivo: "core/ds/fila_enc.c", texto: fonteFilaEnc },
  [Src.SRC_FILA_VET]: { arquivo: "core/ds/fila_vet.c", texto: fonteFilaVet },
  [Src.SRC_LISTA_SIMPLES]: {
    arquivo: "core/ds/lista_simples.c",
    texto: fonteListaSimples,
  },
  [Src.SRC_LISTA_DUPLA]: {
    arquivo: "core/ds/lista_dupla.c",
    texto: fonteListaDupla,
  },
  [Src.SRC_LISTA_CIRCULAR]: {
    arquivo: "core/ds/lista_circular.c",
    texto: fonteListaCircular,
  },
  [Src.SRC_BOLHA]: { arquivo: "core/sort/bolha.c", texto: fonteBolha },
  [Src.SRC_SELECAO]: { arquivo: "core/sort/selecao.c", texto: fonteSelecao },
  [Src.SRC_INSERCAO]: {
    arquivo: "core/sort/insercao.c",
    texto: fonteInsercao,
  },
  [Src.SRC_SHELL]: { arquivo: "core/sort/shell.c", texto: fonteShell },
  [Src.SRC_QUICK]: { arquivo: "core/sort/quick.c", texto: fonteQuick },
  [Src.SRC_MERGE]: { arquivo: "core/sort/merge.c", texto: fonteMerge },
  [Src.SRC_CENA]: { arquivo: "core/sort/cena.c", texto: fonteCena },
  [Src.SRC_ABB]: { arquivo: "core/arvore/abb.c", texto: fonteAbb },
  [Src.SRC_VETOR_ORD]: {
    arquivo: "core/busca/vetor_ord.c",
    texto: fonteVetorOrd,
  },
};

const cache = new Map<number, Fonte>();

export function fonteDe(src: number): Fonte | null {
  const achado = cache.get(src);
  if (achado) return achado;

  const entrada = bruto[src];
  if (!entrada) return null;

  const fonte: Fonte = {
    arquivo: entrada.arquivo,
    linhas: entrada.texto.replace(/\r\n/g, "\n").split("\n"),
  };
  cache.set(src, fonte);
  return fonte;
}
