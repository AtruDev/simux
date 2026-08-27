/* Interpretador do script de operações.
 *
 * Serve para reproduzir um exercício da lista da matéria sem clicar trinta
 * vezes, e é o que torna um estado compartilhável por URL possível mais
 * adiante.
 *
 * A sintaxe é deliberadamente frouxa: uma operação por linha ou separadas por
 * vírgula, com ou sem espaço, em português ou inglês, por extenso ou pela
 * inicial, e vários valores no mesmo verbo. Quem está copiando um exercício
 * não deveria ter que aprender uma linguagem.
 *
 *   empilhar 10          push 10          i 10          i10
 *   inserir 1, 2, 3      i 1 2 3          i1 i2 i3
 *   desempilhar          pop              r             d
 *   # comentário
 */

import { Op } from "../core/ops";

export interface Passo {
  op: Op;
  valor: number;
}

/** Uma linha que o interpretador não entendeu, com o texto original. */
export interface Erro {
  linha: number;
  texto: string;
}

export interface Resultado {
  passos: Passo[];
  erros: Erro[];
}

/* Cada operação aceita vários nomes, e a busca é por igualdade exata — "d" não
 * casa com "desempilhar". Os nomes de família trocada ("empilhar" numa fila)
 * são aceitos de propósito: por baixo é a mesma operação do vtable, e recusar
 * seria pedantismo com quem está copiando um enunciado. */
const VERBOS: Array<{ nomes: string[]; op: Op; precisaValor: boolean }> = [
  {
    nomes: [
      "inserir", "insert", "empilhar", "push", "enfileirar", "enqueue", "add",
      "i", "e",
    ],
    op: Op.OP_PUSH,
    precisaValor: true,
  },
  {
    nomes: [
      "remover", "remove", "desempilhar", "pop", "desenfileirar", "dequeue",
      "r", "d",
    ],
    op: Op.OP_POP,
    precisaValor: false,
  },
  {
    nomes: [
      "consultar", "topo", "top", "peek", "frente", "front", "c", "t", "f",
    ],
    op: Op.OP_TOPO,
    precisaValor: false,
  },
  {
    nomes: ["limpar", "clear", "l"],
    op: Op.OP_LIMPAR,
    precisaValor: false,
  },
];

function acharVerbo(palavra: string) {
  return VERBOS.find((v) => v.nomes.includes(palavra));
}

/** Quebra o texto em comandos, guardando de que linha cada um veio.
 *
 * A quebra é em dois estágios, e não num split só: numerar por pedaço faria
 * "i 1, i 2" gastar dois números, e o erro apontaria para uma linha que não
 * existe no que a pessoa escreveu. */
function comandos(texto: string): Array<{ bruto: string; linha: number }> {
  const saida: Array<{ bruto: string; linha: number }> = [];

  texto.split("\n").forEach((linhaBruta, indice) => {
    const semComentario = linhaBruta.split("#")[0] ?? "";
    for (const pedaco of semComentario.split(/[;,]/)) {
      const bruto = pedaco.trim();
      if (bruto.length > 0) saida.push({ bruto, linha: indice + 1 });
    }
  });

  return saida;
}

export function interpretar(texto: string): Resultado {
  const passos: Passo[] = [];
  const erros: Erro[] = [];

  /* O verbo vale até o fim da linha, para "inserir 1, 2, 3" funcionar — que é
   * como se escreve uma lista. Fora da linha ele não vale: numa linha nova, um
   * número solto é engano, não continuação. */
  let herdado: (typeof VERBOS)[number] | null = null;
  let linhaAnterior = 0;

  for (const { bruto, linha } of comandos(texto)) {
    if (linha !== linhaAnterior) {
      herdado = null;
      linhaAnterior = linha;
    }

    /* "i5" e "i 5" são a mesma coisa: separa letra de número quando vierem
     * grudados, que é como se escreve à mão. */
    const normalizado = bruto
      .toLowerCase()
      .replace(/^([a-zà-ú]+)\s*(-?\d)/u, "$1 $2");
    const partes = normalizado.split(/\s+/);
    const explicito = acharVerbo(partes[0] ?? "");

    const verbo = explicito ?? herdado;
    const resto = explicito ? partes.slice(1) : partes;

    if (!verbo || (!explicito && !verbo.precisaValor)) {
      erros.push({ linha, texto: bruto });
      continue;
    }
    if (explicito) {
      herdado = explicito.precisaValor ? explicito : null;
    }

    if (!verbo.precisaValor) {
      /* Palavra sobrando é prosa e passa: "consultar topo", "remover do
       * topo", "desenfileirar da frente" — é assim que a operação se chama
       * em voz alta, e é o próprio rótulo do botão ao lado.
       *
       * Número sobrando é outra coisa: "desempilhar 3" só pode ser lido como
       * "desempilhe três vezes", que não é o que aconteceria. Aí recusa. */
      if (resto.some((token) => Number.isFinite(Number(token)))) {
        erros.push({ linha, texto: bruto });
      } else {
        passos.push({ op: verbo.op, valor: 0 });
      }
      continue;
    }

    if (resto.length === 0) {
      erros.push({ linha, texto: bruto });
      continue;
    }

    /* "inserir 1 2 3" insere os três, na ordem escrita. */
    const valores = resto.map(Number);
    if (valores.some((v) => !Number.isFinite(v))) {
      erros.push({ linha, texto: bruto });
      continue;
    }
    for (const v of valores) {
      passos.push({ op: verbo.op, valor: Math.trunc(v) });
    }
  }

  return { passos, erros };
}
