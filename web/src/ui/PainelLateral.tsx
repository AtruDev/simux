/* Métricas e log. Os dois leem o mesmo modelo que o canvas desenha, então não
 * existe a possibilidade de o número discordar do desenho. */

import { EvKind, Cnt, Ptr, Tag, STR_CHAVES } from "../core/ops";
import type { Ev } from "../core/bridge";
import {
  alturaDaArvore,
  alvoDe,
  contador,
  maiorCadeia,
  tumulos,
  type Modelo,
} from "../model/modelo";
import { t, type Chave } from "../i18n";
import type { Mundo } from "./Estruturas";

interface PropsMetricas {
  modelo: Modelo;
  mundo: Mundo;
  i: number;
  total: number;
  /* No modo comparar, o nome da implementação vira o título — senão os dois
   * painéis dizem "MÉTRICAS" e não dá para saber qual é qual. */
  titulo?: string;
  /* A estrutura rebalanceia. Só quem rebalanceia mostra a linha de rotações. */
  rotativa?: boolean;
}

export function PainelMetricas({
  modelo,
  mundo,
  i,
  total,
  titulo,
  rotativa,
}: PropsMetricas) {
  const tamanho = contador(modelo, Cnt.CNT_TAMANHO);

  /* As duas implementações contam histórias diferentes de propósito: a
   * encadeada mostra alocações, a com vetor mostra escritas e ocupação. É
   * esse contraste que justifica ver as duas. */
  const linhas: Array<[string, string]> = [
    [t("metrica.tamanho"), String(tamanho)],
  ];

  /* A lista mede o que a caminhada custa: é o número que fica diferente entre
   * a simples e a dupla na mesma operação, e o argumento inteiro de existir
   * mais de uma implementação. */
  if (mundo === "hashEnc" || mundo === "hashAbe") {
    /* O fator de carga primeiro: é o número que resume a tabela inteira, e é
     * dele que sai tudo o que se pode prever sobre ela.
     *
     * Depois, o que cada família tem de próprio. A encadeada mostra a maior
     * cadeia, que é a medida de qualidade do `m` escolhido; a aberta mostra
     * sondagens e túmulos, que são o preço de não alocar nada. */
    const baldes = modelo.vetor?.capacidade ?? 0;

    linhas.length = 0;
    linhas.push([t("metrica.tamanho"), String(tamanho)]);
    linhas.push([t("metrica.baldes"), String(baldes)]);
    linhas.push([
      t("metrica.carga"),
      baldes > 0 ? (tamanho / baldes).toFixed(2) : "—",
    ]);
    linhas.push([
      t("metrica.colisoes"),
      String(contador(modelo, Cnt.CNT_COLISOES)),
    ]);

    if (mundo === "hashEnc") {
      linhas.push([t("metrica.maiorCadeia"), String(maiorCadeia(modelo))]);
      linhas.push([
        t("metrica.alocacoes"),
        String(contador(modelo, Cnt.CNT_ALOCACOES)),
      ]);
    } else {
      linhas.push([
        t("metrica.sondagens"),
        String(contador(modelo, Cnt.CNT_SONDAGENS)),
      ]);
      linhas.push([t("metrica.tumulos"), String(tumulos(modelo, Tag.TAG_LIVRE))]);
    }

    linhas.push([
      t("metrica.comparacoes"),
      String(contador(modelo, Cnt.CNT_COMPARACOES)),
    ]);
  } else if (mundo === "arvore") {
    /* Altura contra altura mínima é o painel inteiro desta estrutura.
     *
     * A mínima é ⌈log₂(n+1)⌉ — a altura que uma árvore com esses n nós teria
     * se estivesse perfeitamente equilibrada. Os dois números lado a lado
     * dizem, sem texto, o quanto esta árvore está torta; e quando a AVL
     * chegar, é a igualdade entre eles que vai ser o argumento dela. */
    const nos = modelo.nos.size;
    const altura = alturaDaArvore(modelo, alvoDe(modelo, Ptr.PTR_RAIZ));
    const ideal = nos === 0 ? 0 : Math.ceil(Math.log2(nos + 1));

    linhas.length = 0;
    linhas.push([t("metrica.tamanho"), String(tamanho)]);
    linhas.push([t("metrica.altura"), String(altura)]);
    linhas.push([t("metrica.alturaIdeal"), String(ideal)]);
    linhas.push([
      t("metrica.comparacoes"),
      String(contador(modelo, Cnt.CNT_COMPARACOES)),
    ]);
    linhas.push([
      t("metrica.alocacoes"),
      String(contador(modelo, Cnt.CNT_ALOCACOES)),
    ]);
    /* Rotações é o preço do equilíbrio, e a linha só aparece em quem paga: na
     * ABB o número seria sempre zero, e um zero permanente não informa nada. */
    if (contador(modelo, Cnt.CNT_ROTACOES) > 0 || rotativa) {
      linhas.push([
        t("metrica.rotacoes"),
        String(contador(modelo, Cnt.CNT_ROTACOES)),
      ]);
    }
  } else if (mundo === "busca") {
    /* Comparações primeiro, e isso é a tela inteira: no modo comparar, o
     * número da sequencial e o da binária lado a lado são o argumento. As
     * escritas vêm logo depois porque a inserção ordenada é o preço pago pela
     * busca barata, e ela também está sendo medida. */
    linhas.length = 0;
    linhas.push([
      t("metrica.comparacoes"),
      String(contador(modelo, Cnt.CNT_COMPARACOES)),
    ]);
    linhas.push([t("metrica.tamanho"), String(tamanho)]);
    linhas.push([
      t("metrica.escritas"),
      String(contador(modelo, Cnt.CNT_ESCRITAS)),
    ]);
    linhas.push([
      t("metrica.capacidade"),
      String(modelo.vetor?.capacidade ?? 0),
    ]);
  } else if (mundo === "ordenacao") {
    /* Comparações e escritas são o que descreve um algoritmo de ordenação, e
     * os dois juntos são o que separa a seleção da bolha: as duas fazem O(n²)
     * comparações, e só uma faz O(n) escritas. Tamanho e ocupação não dizem
     * nada aqui — o vetor não cresce nem encolhe. */
    linhas.length = 0;
    linhas.push([
      t("metrica.comparacoes"),
      String(contador(modelo, Cnt.CNT_COMPARACOES)),
    ]);
    linhas.push([
      t("metrica.escritas"),
      String(contador(modelo, Cnt.CNT_ESCRITAS)),
    ]);
    linhas.push([
      t("metrica.alocacoes"),
      String(contador(modelo, Cnt.CNT_ALOCACOES)),
    ]);
    linhas.push([t("ord.tamanho"), String(modelo.vetor?.capacidade ?? 0)]);
  } else if (mundo === "lista") {
    linhas.push([
      t("metrica.comparacoes"),
      String(contador(modelo, Cnt.CNT_COMPARACOES)),
    ]);
    linhas.push([
      t("metrica.alocacoes"),
      String(contador(modelo, Cnt.CNT_ALOCACOES)),
    ]);
    linhas.push([t("metrica.nos"), String(modelo.nos.size)]);
  } else if (modelo.vetor) {
    const cap = modelo.vetor.capacidade;
    linhas.push([t("metrica.capacidade"), String(cap)]);
    linhas.push([t("metrica.escritas"), String(contador(modelo, Cnt.CNT_ESCRITAS))]);
    linhas.push([
      t("metrica.ocupacao"),
      cap > 0 ? `${Math.round((tamanho / cap) * 100)}%` : "—",
    ]);
  } else {
    linhas.push([t("metrica.alocacoes"), String(contador(modelo, Cnt.CNT_ALOCACOES))]);
    linhas.push([t("metrica.nos"), String(modelo.nos.size)]);
  }

  linhas.push([t("metrica.eventos"), `${i} / ${total}`]);

  return (
    <section className="painel">
      <h2>{titulo ?? t("painel.metricas")}</h2>
      <dl className="metricas">
        {linhas.map(([rotulo, valor]) => (
          <div key={rotulo}>
            <dt>{rotulo}</dt>
            <dd className="mono numero">{valor}</dd>
          </div>
        ))}
      </dl>
    </section>
  );
}

/** Um id de nó como o desenho o mostra, ou NULO quando o C mandou 0. */
function no(id: number): string {
  return id === 0 ? t("log.nulo") : `#${id}`;
}

const NOME_CONTADOR: Partial<Record<number, Chave>> = {
  [Cnt.CNT_TAMANHO]: "metrica.tamanho",
  [Cnt.CNT_ALOCACOES]: "metrica.alocacoes",
  [Cnt.CNT_ESCRITAS]: "metrica.escritas",
  [Cnt.CNT_COMPARACOES]: "metrica.comparacoes",
  [Cnt.CNT_ROTACOES]: "metrica.rotacoes",
  [Cnt.CNT_COLISOES]: "metrica.colisoes",
  [Cnt.CNT_SONDAGENS]: "metrica.sondagens",
};

const NOME_PONTEIRO: Partial<Record<number, Chave>> = {
  [Ptr.PTR_TOPO]: "log.ponteiro",
  [Ptr.PTR_FRENTE]: "log.frente",
  [Ptr.PTR_FIM]: "log.fim",
  [Ptr.PTR_INICIO]: "log.inicio",
  [Ptr.PTR_CURSOR]: "log.cursor",
  [Ptr.PTR_RAIZ]: "log.raiz",
  [Ptr.PTR_BALDE]: "log.balde",
};

/* O mesmo EV_PTR_SET carrega coisas diferentes conforme o mundo, e sem saber
 * qual é não dá para traduzir: no vetor, 0 é um índice legítimo; no
 * encadeado, 0 é NULL. Foi por pouco que isto não virou um log mentindo
 * "NULO" para a célula zero. */
function alvoPonteiro(valor: number, mundo: Mundo): string {
  if (
    mundo === "vetor" ||
    mundo === "ordenacao" ||
    mundo === "busca" ||
    mundo === "hashAbe" ||
    mundo === "hashEnc"
  ) {
    return valor < 0 ? "—" : `[${valor}]`;
  }
  return valor === 0 ? t("log.nulo") : `#${valor}`;
}

/* i, j e mín não passam pelo i18n, e é deliberado: são os identificadores do
 * laço em C, os mesmos que o painel de código está exibindo ao lado. Traduzir
 * "i" faria o log falar de uma variável que não existe no arquivo. É a mesma
 * decisão dos rótulos `topo` e `frente` desenhados no canvas. */
const NOME_CURSOR: Partial<Record<number, string>> = {
  [Ptr.PTR_I]: "i →",
  [Ptr.PTR_J]: "j →",
  [Ptr.PTR_MIN]: "mín →",
};

/** Descreve um evento em português ou inglês, a partir do id — nunca de texto
 * vindo do C, que não devolve texto nenhum. */
export function descrever(ev: Ev, mundo: Mundo): string {
  switch (ev.kind) {
    case EvKind.EV_MSG: {
      const chave = STR_CHAVES[ev.a];
      return chave ? t(chave as Chave) : "";
    }
    case EvKind.EV_NODE_NEW:
      return `${t("log.noCriado")} ${no(ev.a)} = ${ev.b}`;
    case EvKind.EV_NODE_FREE:
      return `${t("log.noLiberado")} ${no(ev.a)}`;
    case EvKind.EV_EDGE_SET:
      /* O mesmo slot quer dizer coisas diferentes conforme o mundo: na lista
       * dupla é prox e ant, na árvore é esquerda e direita. Sem dizer qual,
       * o log de uma remoção com dois filhos fica ilegível. */
      if (mundo === "arvore") {
        const lado = ev.b === 0 ? t("log.esquerda") : t("log.direita");
        return `${no(ev.a)} ${lado} ${no(ev.c)}`;
      }
      return `${no(ev.a)} ${t("log.aresta")} ${no(ev.c)}`;
    case EvKind.EV_PTR_SET: {
      const nome = NOME_PONTEIRO[ev.a];
      const cursor = NOME_CURSOR[ev.a];
      const rotulo = nome ? t(nome) : (cursor ?? `ptr ${ev.a} →`);
      return `${rotulo} ${alvoPonteiro(ev.b, mundo)}`;
    }

    case EvKind.EV_ARR_INIT:
      return `${t("log.vetorInicia")} ${ev.a}`;

    case EvKind.EV_ARR_WRITE:
      return `${t("log.escreve")} [${ev.a}] = ${ev.b}`;

    case EvKind.EV_ARR_READ:
      return `${t("log.le")} [${ev.a}]`;

    case EvKind.EV_ARR_MARK:
      switch (ev.b) {
        case Tag.TAG_LIVRE:
          return `${t("log.marcaLivre")} [${ev.a}]`;
        case Tag.TAG_ORDENADO:
          return `[${ev.a}] ${t("log.marcaOrdenado")}`;
        case Tag.TAG_PIVO:
          /* A mesma marca quer dizer coisas diferentes conforme o mundo: no
           * quicksort ela é o pivô, na busca é a célula que continha a chave.
           * O evento é um só de propósito — inventar TAG_ACHADO seria criar
           * vocabulário para uma diferença que é de leitura, não de dado. */
          return mundo === "busca"
            ? `[${ev.a}] ${t("legenda.achado")}`
            : `[${ev.a}] ${t("log.marcaPivo")}`;
        default:
          return `[${ev.a}]`;
      }

    case EvKind.EV_ARR_COMPARE:
      /* c = 1 é o valor em mãos: b indexa o auxiliar, e escrever "[0]" aqui
       * faria o log dizer que a comparação foi com a célula zero do vetor. */
      return ev.c === 1
        ? `${t("log.compara")} [${ev.a}] ${t("log.emMaos")}`
        : `${t("log.compara")} [${ev.a}] ↔ [${ev.b}]`;

    case EvKind.EV_ARR_SWAP:
      return `${t("log.troca")} [${ev.a}] ↔ [${ev.b}]`;

    case EvKind.EV_ARR_RANGE:
      return `${t("log.faixa")} [${ev.a}…${ev.b}]`;

    case EvKind.EV_AUX_INIT:
      return `${t("log.auxInicia")} ${ev.a}`;

    case EvKind.EV_AUX_WRITE:
      return `${t("log.auxEscreve")} [${ev.a}] = ${ev.b}`;

    case EvKind.EV_PHASE: {
      const chave = STR_CHAVES[ev.a];
      const nome = chave ? t(chave as Chave) : t("log.fase");
      /* O operando da fase é o gap do shell, o número da passada, os extremos
       * do trecho do merge. Só aparece quando existe. */
      if (ev.c !== 0) return `${nome} ${ev.b}…${ev.c}`;
      return ev.b !== 0 ? `${nome} ${ev.b}` : nome;
    }
    case EvKind.EV_NODE_SET:
      return `${no(ev.a)} = ${ev.c}`;

    case EvKind.EV_VISIT:
      return `${t("log.visita")} ${no(ev.a)}`;
    case EvKind.EV_UNVISIT:
      return `${t("log.saiVisita")} ${no(ev.a)}`;
    case EvKind.EV_COUNT: {
      const nome = NOME_CONTADOR[ev.a];
      const rotulo = nome ? t(nome) : t("log.contador");
      return `${rotulo} ${ev.b >= 0 ? "+" : ""}${ev.b}`;
    }
    default:
      return EvKind[ev.kind] ?? String(ev.kind);
  }
}

export function PainelLog({
  eventos,
  mundo,
}: {
  eventos: Ev[];
  mundo: Mundo;
}) {
  return (
    <section className="painel painel-log">
      <h2>{t("painel.log")}</h2>
      {eventos.length === 0 ? (
        <p className="vazio">{t("log.vazio")}</p>
      ) : (
        <ol className="log">
          {eventos.map((ev, k) => (
            <li key={`${k}-${ev.line}`} className={k === 0 ? "atual" : ""}>
              {descrever(ev, mundo)}
            </li>
          ))}
        </ol>
      )}
    </section>
  );
}
