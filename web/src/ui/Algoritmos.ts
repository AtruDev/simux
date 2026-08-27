/* O catálogo da aba de ordenação: o que muda de um algoritmo para o outro.
 *
 * O mesmo papel que Estruturas.ts faz na aba 1. A lista é derivada do enum
 * ALG_* que veio do ids.h, então acrescentar um algoritmo no C faz o seletor
 * crescer sozinho — o que falta aqui é só o que o C não sabe: o arquivo-fonte
 * a exibir, o nome traduzido e o teto de n do modo empírico.
 *
 * O teto existe porque um algoritmo quadrático em n = 25 600 são 327 milhões
 * de comparações. O gráfico precisa das duas famílias no mesmo eixo, e é o
 * teto por algoritmo que deixa cada curva ir até onde ainda faz sentido
 * medir, em vez de o gráfico inteiro parar no limite do mais lento. */

import { Alg, Dist, Src } from "../core/ops";
import type { Chave } from "../i18n";

export interface Algoritmo {
  alg: number;
  nome: Chave;
  /* Qual .c o painel de código exibe enquanto ele roda. */
  src: number;
  /* A identidade de algoritmo é LINHA FINA, nunca preenchimento de barra:
   * é o token que o cabeçalho e a curva do gráfico usam.
   *
   * São seis algoritmos para três matizes, e isso é deliberado — a paleta foi
   * validada para protanopia e deuteranopia com três, e inventar mais três
   * hues exigiria revalidar tudo. O que separa dois algoritmos do mesmo matiz
   * é o TRAÇO, que é a segunda leitura: identidade nunca é só cor.
   *
   * O par (matiz, traço) é da entidade e não muda: esconder uma curva do
   * gráfico não repinta as que sobraram. */
  token: "--alg-1" | "--alg-2" | "--alg-3";
  traco: "solido" | "tracejado" | "pontilhado";
  /* Maior n que o modo empírico mede neste algoritmo. */
  tetoBench: number;
  /* Complexidade média, para a legenda. Texto do i18n, não string aqui. */
  ordem: Chave;
}

export const ALGORITMOS: Algoritmo[] = [
  {
    alg: Alg.ALG_BOLHA,
    nome: "alg.bolha",
    src: Src.SRC_BOLHA,
    token: "--alg-2",
    traco: "solido",
    tetoBench: 4096,
    ordem: "ordem.quadratica",
  },
  {
    alg: Alg.ALG_SELECAO,
    nome: "alg.selecao",
    src: Src.SRC_SELECAO,
    token: "--alg-2",
    traco: "tracejado",
    tetoBench: 4096,
    ordem: "ordem.quadratica",
  },
  {
    alg: Alg.ALG_INSERCAO,
    nome: "alg.insercao",
    src: Src.SRC_INSERCAO,
    token: "--alg-2",
    traco: "pontilhado",
    tetoBench: 4096,
    ordem: "ordem.quadratica",
  },
  {
    alg: Alg.ALG_SHELL,
    nome: "alg.shell",
    src: Src.SRC_SHELL,
    token: "--alg-3",
    traco: "solido",
    tetoBench: 32768,
    ordem: "ordem.shell",
  },
  {
    alg: Alg.ALG_QUICK,
    nome: "alg.quick",
    src: Src.SRC_QUICK,
    token: "--alg-1",
    traco: "solido",
    /* O quicksort com pivô ingênuo cai em O(n²) nas distribuições que existem
     * justamente para derrubá-lo. O teto é o do quadrático, não o do n log n
     * — medir 25 600 em "poucos valores distintos" travaria a aba. */
    tetoBench: 8192,
    ordem: "ordem.linearitmica",
  },
  {
    alg: Alg.ALG_MERGE,
    nome: "alg.merge",
    src: Src.SRC_MERGE,
    token: "--alg-1",
    traco: "tracejado",
    tetoBench: 32768,
    ordem: "ordem.linearitmica",
  },
];

export function algoritmoDe(alg: number): Algoritmo {
  return ALGORITMOS.find((a) => a.alg === alg) ?? ALGORITMOS[0]!;
}

/* ---- distribuições ------------------------------------------------------ */

export interface Distribuicao {
  dist: number;
  nome: Chave;
  /* Por que ela existe — aparece como dica, e é metade do conteúdo da aba. */
  porque: Chave;
}

export const DISTRIBUICOES: Distribuicao[] = [
  {
    dist: Dist.DIST_ALEATORIO,
    nome: "dist.aleatorio",
    porque: "dist.aleatorioPorque",
  },
  {
    dist: Dist.DIST_QUASE_ORDENADO,
    nome: "dist.quaseOrdenado",
    porque: "dist.quaseOrdenadoPorque",
  },
  {
    dist: Dist.DIST_INVERSO,
    nome: "dist.inverso",
    porque: "dist.inversoPorque",
  },
  {
    dist: Dist.DIST_POUCOS_DISTINTOS,
    nome: "dist.poucosDistintos",
    porque: "dist.poucosDistintosPorque",
  },
  {
    dist: Dist.DIST_ORDENADO,
    nome: "dist.ordenado",
    porque: "dist.ordenadoPorque",
  },
  {
    dist: Dist.DIST_MANUAL,
    nome: "dist.manual",
    porque: "dist.manualPorque",
  },
];

export function distribuicaoDe(dist: number): Distribuicao {
  return DISTRIBUICOES.find((d) => d.dist === dist) ?? DISTRIBUICOES[0]!;
}

/**
 * Lê a lista de números que o campo da distribuição manual aceita.
 *
 * Separadores livres — vírgula, espaço, quebra de linha —, porque o valor
 * típico vem colado de um enunciado de exercício. Devolve null quando não há
 * número nenhum, que é o estado em que o botão fica desabilitado em vez de a
 * chamada falhar no C.
 */
export function lerManual(texto: string, maximo: number): number[] | null {
  const numeros = texto
    .split(/[^-\d]+/)
    .filter((p) => p.length > 0 && p !== "-")
    .map((p) => Number(p) | 0);

  if (numeros.length === 0) return null;
  return numeros.slice(0, maximo);
}
