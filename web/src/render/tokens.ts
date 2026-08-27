/* Lê a paleta do CSS. O renderizador não define cor nenhuma: se existisse uma
 * segunda paleta aqui, ela divergiria da de tokens.css na primeira mudança. */

export interface Paleta {
  canvas: string;
  bg2: string;
  bg3: string;
  linha: string;
  linha2: string;
  fg: string;
  fg2: string;
  fg3: string;
  acento: string;
  acentoAlto: string;
  acentoFraco: string;
  stDone: string;
  stSwap: string;
  stCompare: string;
  stPivot: string;
  stAux: string;
  alg1: string;
  alg2: string;
  alg3: string;
  mono: string;
}

export function lerPaleta(el: Element): Paleta {
  const cs = getComputedStyle(el);
  const v = (nome: string) => cs.getPropertyValue(nome).trim();

  return {
    canvas: v("--canvas"),
    bg2: v("--bg-2"),
    bg3: v("--bg-3"),
    linha: v("--line"),
    linha2: v("--line-2"),
    fg: v("--fg"),
    fg2: v("--fg-2"),
    fg3: v("--fg-3"),
    acento: v("--accent"),
    acentoAlto: v("--accent-hi"),
    acentoFraco: v("--accent-dim"),
    stDone: v("--st-done"),
    stSwap: v("--st-swap"),
    stCompare: v("--st-compare"),
    stPivot: v("--st-pivot"),
    stAux: v("--st-aux"),
    alg1: v("--alg-1"),
    alg2: v("--alg-2"),
    alg3: v("--alg-3"),
    mono: v("--fonte-mono"),
  };
}
