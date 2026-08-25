/* O painel de código mostra o .c de verdade, importado cru.
 *
 * Não é pseudocódigo reescrito para a tela: é o mesmo arquivo que compilou
 * para o wasm que acabou de rodar. É por isso que a linha destacada pode ser
 * confiada — ela vem do __LINE__ que a macro TR gravou. */

import fonteApi from "../../../core/api/api.c?raw";
import fontePilhaEnc from "../../../core/ds/pilha_enc.c?raw";
import { Src } from "../core/ops";

export interface Fonte {
  arquivo: string;
  linhas: string[];
}

const bruto: Partial<Record<number, { arquivo: string; texto: string }>> = {
  [Src.SRC_API]: { arquivo: "core/api/api.c", texto: fonteApi },
  [Src.SRC_PILHA_ENC]: { arquivo: "core/ds/pilha_enc.c", texto: fontePilhaEnc },
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
