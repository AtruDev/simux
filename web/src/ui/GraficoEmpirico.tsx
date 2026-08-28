/* O modo empírico: comparações medidas × n, com as curvas teóricas por cima.
 *
 * É a peça que muda o que a aba diz. Sem ela, o projeto afirma "implementei os
 * seis algoritmos". Com ela, afirma "medi, e a curva bate com a teoria" — e
 * mostra, de quebra, que a inserção ganha do quicksort em vetor pequeno ou
 * quase ordenado, que é conteúdo de prova.
 *
 * Cada ponto é uma execução de verdade, chamando ds_bench com o trace
 * desligado. Não há fórmula nenhuma do lado do JavaScript: o número plotado é
 * o que o contador do C viu acontecer.
 *
 * O desenho segue as três regras de cor do projeto. As curvas são LINHA FINA
 * em cor de algoritmo, que é onde a identidade de algoritmo mora; as teóricas
 * são tinta neutra, porque não são séries e não podem competir com os dados.
 * Como são seis algoritmos para três matizes validados, cada um carrega também
 * um traço próprio, e todos ficam rotulados na ponta: identidade nunca é só
 * cor. */

import { useCallback, useEffect, useRef, useState } from "react";

import { medir } from "../core/bridge";
import { t, type Chave } from "../i18n";
import { ALGORITMOS, type Algoritmo } from "./Algoritmos";

/* Os n medidos, dobrando. Dobrar é o que faz a leitura ser direta: num
 * algoritmo quadrático o número quadruplica de um ponto para o outro, e num
 * n log n pouco mais que dobra — dá para ver sem calcular nada. */
const TAMANHOS = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768];

const LARGURA = 720;
const ALTURA = 300;
const MARGEM = { topo: 16, dir: 96, baixo: 34, esq: 58 };

const AREA_L = LARGURA - MARGEM.esq - MARGEM.dir;
const AREA_A = ALTURA - MARGEM.topo - MARGEM.baixo;

/** Um ponto medido. */
interface Ponto {
  n: number;
  comparacoes: number;
  escritas: number;
}

type Serie = { alg: number; pontos: Ponto[] };

type Metrica = "comparacoes" | "escritas";

const TRACEJADO: Record<Algoritmo["traco"], string | undefined> = {
  solido: undefined,
  tracejado: "6 4",
  pontilhado: "2 3",
};

interface Props {
  dist: number;
  semente: number;
}

export function GraficoEmpirico({ dist, semente }: Props) {
  const [series, setSeries] = useState<Serie[]>([]);
  const [rodando, setRodando] = useState(false);
  const [metrica, setMetrica] = useState<Metrica>("comparacoes");
  const cancelar = useRef(false);

  /* Trocar de distribuição invalida o que estava medido: as curvas de
   * "aleatório" e de "já ordenado" contam histórias opostas, e deixá-las na
   * tela com o seletor apontando para outra coisa seria mentir. */
  useEffect(() => {
    setSeries([]);
  }, [dist, semente]);

  useEffect(() => {
    return () => {
      cancelar.current = true;
    };
  }, []);

  /* Mede tudo, cedendo o controle entre as medidas.
   *
   * ds_bench é síncrono e uma execução quadrática em n = 4096 leva um tempo
   * visível. Sem o await no meio, a página congelaria até o fim da bateria e o
   * botão nem chegaria a mudar para "medindo…". */
  const rodar = useCallback(async () => {
    setRodando(true);
    cancelar.current = false;
    const acumulado: Serie[] = [];

    for (const a of ALGORITMOS) {
      if (a.semBench) continue;

      const pontos: Ponto[] = [];

      for (const n of TAMANHOS) {
        if (cancelar.current) return;
        if (n > a.tetoBench) break;

        const medida = medir(a.alg, n, dist, semente);
        if (medida) {
          pontos.push({
            n,
            comparacoes: medida.comparacoes,
            escritas: medida.escritas,
          });
        }
        await new Promise((r) => setTimeout(r, 0));
      }

      acumulado.push({ alg: a.alg, pontos });
      /* Uma cópia por algoritmo: as curvas aparecem uma a uma, e a espera
       * deixa de ser uma tela parada. */
      setSeries([...acumulado]);
    }

    setRodando(false);
  }, [dist, semente]);

  const valorDe = (p: Ponto) =>
    metrica === "comparacoes" ? p.comparacoes : p.escritas;

  const comDados = series.filter((s) => s.pontos.length > 0);
  const temDados = comDados.length > 0;

  /* Escala log nos dois eixos.
   *
   * Não é preferência: em escala linear a curva do mergesort vira uma linha
   * colada no zero enquanto a da bolha ocupa o gráfico inteiro, e a única
   * coisa legível seria "uma é maior". Em log-log, cada complexidade é uma
   * reta com inclinação própria, e comparar inclinações é exatamente comparar
   * ordens de crescimento. */
  let maiorN = 1;
  let maiorV = 1;
  for (const s of comDados) {
    for (const p of s.pontos) {
      if (p.n > maiorN) maiorN = p.n;
      const v = valorDe(p);
      if (v > maiorV) maiorV = v;
    }
  }
  const menorN = TAMANHOS[0]!;

  const px = (n: number) =>
    MARGEM.esq +
    (Math.log2(n) - Math.log2(menorN)) /
      Math.max(1, Math.log2(maiorN) - Math.log2(menorN)) *
      AREA_L;

  const py = (v: number) =>
    MARGEM.topo +
    AREA_A -
    (Math.log2(Math.max(v, 1)) / Math.max(1, Math.log2(maiorV))) * AREA_A;

  const caminho = (pontos: Ponto[]) =>
    pontos
      .map((p, k) => `${k === 0 ? "M" : "L"} ${px(p.n)} ${py(valorDe(p))}`)
      .join(" ");

  return (
    <section className="painel painel-grafico">
      <header className="grafico-cabecalho">
        <h2>{t("painel.empirico")}</h2>

        <label className="velocidade">
          <span className="rotulo">{t("empirico.metrica")}</span>
          <select
            value={metrica}
            onChange={(e) => setMetrica(e.target.value as Metrica)}
          >
            <option value="comparacoes">{t("empirico.comparacoes")}</option>
            <option value="escritas">{t("empirico.escritas")}</option>
          </select>
        </label>

        <button type="button" onClick={() => void rodar()} disabled={rodando}>
          {rodando ? t("empirico.rodando") : t("empirico.rodar")}
        </button>
      </header>

      {!temDados ? (
        <p className="vazio">{t("empirico.vazio")}</p>
      ) : (
        <>
          <svg
            className="grafico"
            viewBox={`0 0 ${LARGURA} ${ALTURA}`}
            role="img"
            aria-label={t("painel.empirico")}
          >
            <Eixos maiorN={maiorN} maiorV={maiorV} px={px} py={py} />

            {/* As teóricas primeiro, para as medidas ficarem por cima. São
                ancoradas no primeiro ponto do algoritmo mais lento medido:
                o que se compara é a INCLINAÇÃO, não a altura. */}
            <Teoricas
              menorN={menorN}
              maiorN={maiorN}
              ancora={ancoraDe(comDados, valorDe)}
              px={px}
              py={py}
            />

            {comDados.map((s) => {
              const a = ALGORITMOS.find((x) => x.alg === s.alg)!;

              return (
                <g key={s.alg}>
                  <path
                    d={caminho(s.pontos)}
                    fill="none"
                    stroke={`var(${a.token})`}
                    strokeWidth={2}
                    strokeDasharray={TRACEJADO[a.traco]}
                    strokeLinejoin="round"
                  />
                  {s.pontos.map((p) => (
                    <circle
                      key={p.n}
                      cx={px(p.n)}
                      cy={py(valorDe(p))}
                      r={3}
                      fill={`var(${a.token})`}
                    />
                  ))}
                </g>
              );
            })}
            {/* Rótulo na ponta de cada curva: com seis delas, procurar na
                legenda qual é qual custa mais que ler o nome onde a linha
                acaba. Duas curvas da mesma ordem terminam quase na mesma
                altura — bolha e seleção fazem as duas n²/2 comparações —,
                então os rótulos são afastados antes de serem desenhados. */}
            {semColisao(
              comDados.map((s) => {
                const a = ALGORITMOS.find((x) => x.alg === s.alg)!;
                const ultimo = s.pontos[s.pontos.length - 1]!;
                return {
                  chave: String(s.alg),
                  texto: t(a.nome),
                  x: px(ultimo.n) + 8,
                  y: py(valorDe(ultimo)) + 4,
                };
              }),
            ).map((r) => (
              <text className="grafico-rotulo" key={r.chave} x={r.x} y={r.y}>
                {r.texto}
              </text>
            ))}
          </svg>

          <Tabela series={comDados} metrica={metrica} valorDe={valorDe} />

          <p className="dica">{t("empirico.explica")}</p>
          <p className="dica">{t("empirico.limite")}</p>
        </>
      )}
    </section>
  );
}

/** Rótulo de ponta de curva, antes de saber se cabe onde quer ficar. */
interface Rotulo {
  chave: string;
  texto: string;
  x: number;
  y: number;
}

/* Afasta rótulos que cairiam um por cima do outro.
 *
 * Duas curvas da mesma ordem de crescimento terminam quase na mesma altura, e
 * é exatamente aí que saber qual é qual mais importa. Só colidem os que
 * terminam na mesma coluna — curvas com tetos de n diferentes acabam em x
 * diferentes e não se estorvam. */
function semColisao(rotulos: Rotulo[]): Rotulo[] {
  const ALTURA_LINHA = 12;
  const porColuna = new Map<number, Rotulo[]>();

  for (const r of rotulos) {
    const coluna = Math.round(r.x / 24);
    const lista = porColuna.get(coluna) ?? [];
    lista.push(r);
    porColuna.set(coluna, lista);
  }

  const saida: Rotulo[] = [];
  for (const lista of porColuna.values()) {
    lista.sort((a, b) => a.y - b.y);

    let anterior = -Infinity;
    for (const r of lista) {
      const y = Math.max(r.y, anterior + ALTURA_LINHA);
      saida.push({ ...r, y });
      anterior = y;
    }
  }
  return saida;
}

/** O primeiro ponto medido, para as curvas teóricas partirem de algum lugar. */
function ancoraDe(
  series: Serie[],
  valorDe: (p: Ponto) => number,
): { n: number; v: number } | null {
  let melhor: { n: number; v: number } | null = null;

  for (const s of series) {
    const p = s.pontos[0];
    if (!p) continue;
    const v = valorDe(p);
    if (v <= 0) continue;
    if (melhor === null || v > melhor.v) melhor = { n: p.n, v };
  }
  return melhor;
}

/** n² e n log n em tinta neutra: são referência, não série. */
function Teoricas({
  menorN,
  maiorN,
  ancora,
  px,
  py,
}: {
  menorN: number;
  maiorN: number;
  ancora: { n: number; v: number } | null;
  px: (n: number) => number;
  py: (v: number) => number;
}) {
  if (!ancora) return null;

  const amostras: number[] = [];
  for (let n = menorN; n <= maiorN; n *= 2) amostras.push(n);
  if (amostras[amostras.length - 1] !== maiorN) amostras.push(maiorN);

  const quad = (n: number) => ancora.v * (n / ancora.n) ** 2;
  const linlog = (n: number) =>
    (ancora.v * (n * Math.log2(n))) / (ancora.n * Math.log2(ancora.n));

  const desenhar = (f: (n: number) => number) =>
    amostras
      .map((n, k) => `${k === 0 ? "M" : "L"} ${px(n)} ${py(f(n))}`)
      .join(" ");

  return (
    <g className="grafico-teorica">
      <path d={desenhar(quad)} fill="none" strokeWidth={1} />
      <path d={desenhar(linlog)} fill="none" strokeWidth={1} />
      {/* As duas legendas teóricas ficam presas ao topo e à base do próprio
          par, e não na ponta das curvas: elas competiriam com o nome do
          algoritmo, que é o rótulo que o leitor está procurando. */}
      <text x={px(maiorN) + 6} y={py(quad(maiorN)) - 6}>
        n²
      </text>
      <text x={px(maiorN) + 6} y={py(linlog(maiorN)) + 14}>
        n log n
      </text>
    </g>
  );
}

function Eixos({
  maiorN,
  maiorV,
  px,
  py,
}: {
  maiorN: number;
  maiorV: number;
  px: (n: number) => number;
  py: (v: number) => number;
}) {
  const marcasN = TAMANHOS.filter((n) => n <= maiorN);

  const marcasV: number[] = [];
  for (let v = 100; v <= maiorV; v *= 10) marcasV.push(v);

  return (
    <g className="grafico-eixos">
      {marcasV.map((v) => (
        <g key={v}>
          <line
            x1={MARGEM.esq}
            x2={LARGURA - MARGEM.dir}
            y1={py(v)}
            y2={py(v)}
          />
          <text className="grafico-tick" x={MARGEM.esq - 8} y={py(v) + 4}>
            {compacto(v)}
          </text>
        </g>
      ))}
      {marcasN.map((n) => (
        <text
          key={n}
          className="grafico-tick meio"
          x={px(n)}
          y={ALTURA - MARGEM.baixo + 16}
        >
          {potencia(n)}
        </text>
      ))}
      <text className="grafico-tick meio" x={px(maiorN)} y={ALTURA - 6}>
        n
      </text>
    </g>
  );
}

/** A tabela existe para o gráfico não ser a única forma de ler o dado. */
function Tabela({
  series,
  metrica,
  valorDe,
}: {
  series: Serie[];
  metrica: Metrica;
  valorDe: (p: Ponto) => number;
}) {
  const ns = TAMANHOS.filter((n) =>
    series.some((s) => s.pontos.some((p) => p.n === n)),
  );
  const rotulo: Chave =
    metrica === "comparacoes" ? "empirico.comparacoes" : "empirico.escritas";

  return (
    <details className="tabela-medida">
      <summary>{t(rotulo)}</summary>
      <div className="rolagem">
        <table className="mono">
          <thead>
            <tr>
              <th scope="col">n</th>
              {series.map((s) => {
                const a = ALGORITMOS.find((x) => x.alg === s.alg)!;
                return (
                  <th key={s.alg} scope="col">
                    {t(a.nome)}
                  </th>
                );
              })}
            </tr>
          </thead>
          <tbody>
            {ns.map((n) => (
              <tr key={n}>
                <th scope="row">{n}</th>
                {series.map((s) => {
                  const p = s.pontos.find((x) => x.n === n);
                  return (
                    <td key={s.alg} className="numero">
                      {p ? valorDe(p).toLocaleString() : "—"}
                    </td>
                  );
                })}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </details>
  );
}

/** 1000000 vira 1M. Números longos no eixo empurram o gráfico para o lado. */
function compacto(v: number): string {
  if (v >= 1_000_000) return `${Math.round(v / 100_000) / 10}M`;
  if (v >= 1_000) return `${Math.round(v / 100) / 10}k`;
  return String(v);
}

/* O eixo dos n tem escala própria porque os valores são potências de dois:
 * 4096 dividido por mil vira "4.1k", que é feio e sugere um número redondo que
 * não é. Dividido por 1024 vira "4k", que é como esses tamanhos se chamam. */
function potencia(n: number): string {
  return n >= 1024 ? `${n / 1024}k` : String(n);
}
