/* A aba 2: os seis algoritmos de ordenação sobre o mesmo vetor.
 *
 * Quase nada aqui é novo. O Player é o mesmo da aba 1, com as mesmas trilhas
 * que o modo comparar estreou — o modo corrida É o modo comparar, com os
 * algoritmos no lugar das implementações. O modelo é o mesmo, o aplicar.ts é
 * o mesmo, e o que mudou foi um renderizador: o VetorView desenha células com
 * o número dentro, e ordenar precisa de forma, não de número.
 *
 * A ordem das operações também é a de sempre: o C executa na hora e devolve o
 * trace inteiro; a animação só decide a que velocidade assistir. */

import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  useSyncExternalStore,
} from "react";

import {
  bufferEntrada,
  chamar,
  selecionarSlot,
  sessaoNova,
  slots,
} from "../core/bridge";
import { Dist, Op, Status, Tipo, STR_CHAVES } from "../core/ops";
import { ERR_CHAVES } from "../core/ops";
import { Player, type Operacao } from "../core/player";
import { t, type Chave } from "../i18n";
import { OrdenacaoView } from "../render/ordenacaoView";
import {
  ALGORITMOS,
  DISTRIBUICOES,
  algoritmoDe,
  distribuicaoDe,
  lerManual,
  type Algoritmo,
} from "./Algoritmos";
import { GraficoEmpirico } from "./GraficoEmpirico";
import { PainelCodigo } from "./PainelCodigo";
import { PainelLog, PainelMetricas } from "./PainelLateral";
import { Transporte } from "./Transporte";

/* O plano pede 5 a 200 no modo animado. O piso não é enfeite: com menos de
 * cinco elementos o quicksort não chega a particionar duas vezes. */
const N_MIN = 5;
const N_MAX = 200;

/* No modo corrida o vetor encolhe. São seis trilhas em seis canvas, e 200
 * barras em cada um viram seis borrões — o que se quer ver ali é a silhueta
 * de um terminando antes do outro. */
const N_MAX_CORRIDA = 60;

export function AbaOrdenacao() {
  const player = useMemo(() => new Player(), []);
  const foto = useSyncExternalStore(player.assinar, player.ler);

  const [erro, setErro] = useState<string | null>(null);
  const [alg, setAlg] = useState<number>(ALGORITMOS[0]!.alg);
  const [dist, setDist] = useState<number>(Dist.DIST_ALEATORIO);
  const [n, setN] = useState(24);
  const [semente, setSemente] = useState(1);
  const [corrida, setCorrida] = useState(false);
  const [manual, setManual] = useState("5, 3, 8, 1, 9, 2");

  const algoritmo = algoritmoDe(alg);
  const distribuicao = distribuicaoDe(dist);

  /* As trilhas em cena: um algoritmo, ou todos eles sobre o mesmo vetor.
   *
   * O core tem dois slots de sessão, e a corrida quer seis. Em vez de mexer
   * na fronteira, ela roda um algoritmo de cada vez no slot 0, guarda o trace
   * e recomeça — o vetor inicial é o mesmo porque a semente é a mesma, que é
   * exatamente para isso que a semente existe. O Player recebe os seis traces
   * como seis trilhas e os toca no mesmo relógio. */
  const trilhas: Algoritmo[] = useMemo(
    () => (corrida ? ALGORITMOS : [algoritmo]),
    [corrida, algoritmo],
  );

  const refCanvas = useRef<Array<HTMLCanvasElement | null>>([]);

  /* ---- a cena --------------------------------------------------------- */

  /* Escreve os valores digitados no buffer de entrada do core.
   *
   * É a fronteira de dados: nem string, nem parser em C. O JS escreve inteiros
   * direto na heap do wasm, e OP_GERAR com DIST_MANUAL os lê de lá. */
  const enviarManual = useCallback((quantos: number): boolean => {
    const valores = lerManual(manual, quantos);
    if (valores === null) return false;

    const destino = bufferEntrada(valores.length);
    if (!destino) return false;
    destino.set(valores);
    return true;
  }, [manual]);

  /* Monta a linha do tempo inteira: uma trilha por algoritmo em cena, cada
   * uma com o mesmo vetor inicial e o seu próprio trace de ordenação. */
  const montar = useCallback(
    (ordenar: boolean) => {
      const quantos = corrida ? Math.min(n, N_MAX_CORRIDA) : n;
      const emCena = corrida ? ALGORITMOS : [algoritmoDe(alg)];
      const distEfetiva = dist;

      if (distEfetiva === Dist.DIST_MANUAL && !enviarManual(quantos)) {
        setErro(t("ERR_ARG_INVALIDO"));
        return;
      }

      const cena: Operacao = [];
      const ordenacao: Operacao = [];
      let codigo = Status.OK;

      emCena.forEach((a, k) => {
        /* Cada trilha ocupa um slot, e os slots do core são dois. Acima disso
         * elas se revezam no slot 0: uma trilha só precisa do core enquanto
         * está gerando o próprio trace, e o trace já saiu de lá quando a
         * próxima começa. */
        selecionarSlot(Math.min(k, slots() - 1));

        const criacao = sessaoNova(Tipo.TIPO_ORDENACAO, quantos);
        const geracao = chamar(Op.OP_GERAR, quantos, distEfetiva, semente);
        cena.push([...criacao, ...geracao.eventos]);

        if (ordenar) {
          const saida = chamar(Op.OP_ORDENAR, a.alg);
          ordenacao.push(saida.eventos);
          if (k === 0) codigo = saida.erro;
        } else {
          ordenacao.push([]);
        }
      });

      /* A cena é uma operação, a ordenação é outra: é o que faz o transporte
       * começar com o vetor já na tela e animar só o algoritmo.
       *
       * E o cursor volta para o fim da cena, não para o zero: as escritas que
       * montam o vetor inicial são estado, não trabalho de ordenar. Assistir a
       * elas seria assistir ao gerador. */
      player.carregarTrilhas(ordenar ? [cena, ordenacao] : [cena]);
      if (ordenar) {
        player.irPara(cena[0]?.length ?? 0);
        player.play();
      }
      setErro(codigo === Status.OK ? null : textoDoErro(codigo));
    },
    [player, corrida, n, alg, dist, semente, enviarManual],
  );

  /* Trocar qualquer coisa da cena refaz o vetor, sem ordenar. */
  useEffect(() => {
    try {
      montar(false);
    } catch (e) {
      setErro(String(e));
    }
    /* `montar` já depende de tudo o que define a cena; o texto do campo
     * manual, não — ele só vale quando o botão é apertado, senão o vetor
     * mudaria a cada tecla digitada. */
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [player, corrida, n, alg, dist, semente]);

  /* ---- laço de animação ----------------------------------------------- */

  useEffect(() => player.iniciarLaco(), [player]);

  useEffect(() => {
    const views = trilhas.map((_, k) => {
      const canvas = refCanvas.current[k];
      return canvas ? new OrdenacaoView(canvas) : null;
    });

    const solta = player.aoQuadro(() => {
      views.forEach((view, k) => view?.desenhar(player.estadoDe(k)));
    });

    return () => {
      solta();
      for (const view of views) view?.destruir();
    };
  }, [player, trilhas]);

  /* ---- atalhos --------------------------------------------------------- */

  useEffect(() => {
    function aoTeclar(e: KeyboardEvent) {
      const alvo = e.target as HTMLElement | null;
      if (alvo && /^(INPUT|SELECT|TEXTAREA)$/.test(alvo.tagName)) return;

      switch (e.key) {
        case " ":
          e.preventDefault();
          player.alternar();
          break;
        case "ArrowLeft":
          e.preventDefault();
          player.passo(e.shiftKey ? -10 : -1);
          break;
        case "ArrowRight":
          e.preventDefault();
          player.passo(e.shiftKey ? 10 : 1);
          break;
        default:
          break;
      }
    }
    window.addEventListener("keydown", aoTeclar);
    return () => window.removeEventListener("keydown", aoTeclar);
  }, [player]);

  /* ---- render ---------------------------------------------------------- */

  const faseDaPrimeira = player.estadoDe(0).fase;

  return (
    <div className="grade">
      <aside className="coluna esquerda">
        <section className="painel">
          <h2>{t("painel.algoritmo")}</h2>
          <div className="lista-estruturas">
            {ALGORITMOS.map((a) => (
              <label
                key={a.alg}
                className={
                  trilhas.some((x) => x.alg === a.alg)
                    ? "opcao em-cena"
                    : "opcao"
                }
              >
                <input
                  type="radio"
                  name="algoritmo"
                  checked={alg === a.alg}
                  onChange={() => setAlg(a.alg)}
                />
                <span>{t(a.nome)}</span>
                {/* Identidade de algoritmo é linha fina, nunca preenchimento:
                    é a régua ao lado do nome, e a mesma cor volta como curva
                    no gráfico empírico. */}
                <span
                  className="regua-alg"
                  style={{ background: `var(${a.token})` }}
                  aria-hidden="true"
                />
              </label>
            ))}
          </div>

          <label className="opcao opcao-comparar">
            <input
              type="checkbox"
              checked={corrida}
              onChange={(e) => setCorrida(e.target.checked)}
            />
            <span>{t("ord.corrida")}</span>
          </label>

          <p className="dica mono">{t(algoritmo.ordem)}</p>
        </section>

        <section className="painel">
          <h2>{t("painel.cena")}</h2>

          <div className="campo campo-capacidade">
            <label htmlFor="ord-n">{t("ord.tamanho")}</label>
            <input
              id="ord-n"
              className="mono"
              type="number"
              min={N_MIN}
              max={N_MAX}
              value={n}
              onChange={(e) => {
                const v = Number(e.target.value);
                if (v >= N_MIN && v <= N_MAX) setN(v);
              }}
            />
          </div>

          <div className="campo campo-capacidade">
            <label htmlFor="ord-dist">{t("ord.distribuicao")}</label>
            <select
              id="ord-dist"
              value={dist}
              onChange={(e) => setDist(Number(e.target.value))}
            >
              {DISTRIBUICOES.map((d) => (
                <option key={d.dist} value={d.dist}>
                  {t(d.nome)}
                </option>
              ))}
            </select>
          </div>

          {/* Por que a distribuição existe é metade do conteúdo da aba: sem
              isso ela é um menu de enfeites. */}
          <p className="dica">{t(distribuicao.porque)}</p>

          {dist === Dist.DIST_MANUAL ? (
            <div className="campo campo-manual">
              <label htmlFor="ord-manual">{t("ord.valores")}</label>
              <textarea
                id="ord-manual"
                className="mono"
                rows={2}
                value={manual}
                onChange={(e) => setManual(e.target.value)}
              />
              <p className="dica">{t("ord.valoresAjuda")}</p>
            </div>
          ) : (
            <div className="campo campo-capacidade">
              <label htmlFor="ord-semente">{t("ord.semente")}</label>
              <input
                id="ord-semente"
                className="mono"
                type="number"
                min={1}
                value={semente}
                onChange={(e) => setSemente(Math.max(1, Number(e.target.value)))}
              />
              <button
                type="button"
                className="secundario"
                title={t("ord.novaSemente")}
                onClick={() => setSemente(1 + Math.floor(Math.random() * 9999))}
              >
                ⚄
              </button>
            </div>
          )}

          <div className="operacoes">
            <button type="button" onClick={() => montar(true)}>
              {t("ord.ordenar")}
            </button>
            <button
              type="button"
              className="secundario"
              onClick={() => montar(false)}
            >
              {t("ord.gerar")}
            </button>
          </div>

          {erro !== null && (
            <p className="erro">
              {t("app.erro")}: {erro}
            </p>
          )}
        </section>

        <section className="painel">
          <h2>{t("painel.fase")}</h2>
          <p className="fase mono">
            {faseDaPrimeira ? textoDaFase(faseDaPrimeira) : t("ord.semFase")}
          </p>
        </section>

        <Legenda />

        {trilhas.map((faixa, k) => (
          <PainelMetricas
            key={faixa.alg}
            modelo={player.estadoDe(k)}
            mundo="ordenacao"
            i={foto.i}
            total={foto.total}
            titulo={trilhas.length > 1 ? t(faixa.nome) : undefined}
          />
        ))}
      </aside>

      <main className="coluna centro">
        <div className={corrida ? "faixas corrida" : "faixas"}>
          {trilhas.map((faixa, k) => (
            <div className="faixa" key={faixa.alg}>
              {trilhas.length > 1 && (
                <span className="faixa-nome">
                  <span
                    className="regua-alg"
                    style={{ background: `var(${faixa.token})` }}
                    aria-hidden="true"
                  />
                  {t(faixa.nome)}
                </span>
              )}
              <canvas
                ref={(el) => {
                  refCanvas.current[k] = el;
                }}
                className="canvas"
              />
            </div>
          ))}
        </div>
        <Transporte
          player={player}
          i={foto.i}
          total={foto.total}
          tocando={foto.tocando}
          velocidade={foto.velocidade}
        />
        <GraficoEmpirico dist={dist} semente={semente} />
      </main>

      <aside className="coluna direita">
        {/* No modo corrida o painel de código seria seis arquivos, e a tela
            não tem altura para isso. Ele mostra o algoritmo selecionado, que
            é o que o seletor da esquerda continua apontando. */}
        <PainelCodigo
          src={player.estadoDe(0).fonte?.src ?? algoritmo.src}
          linha={player.estadoDe(0).fonte?.linha ?? 0}
        />
        {!corrida && (
          <PainelLog eventos={player.historico(12)} mundo="ordenacao" />
        )}
      </aside>
    </div>
  );
}

/** A legenda fica sempre visível: nenhum estado é distinguido só por matiz, e
 * quem lê a tela precisa da chave para as duas leituras — cor e traço. */
function Legenda() {
  const itens: Array<[Chave, string, boolean]> = [
    ["legenda.comparando", "--st-compare", false],
    ["legenda.escrita", "--st-swap", false],
    ["legenda.ordenado", "--st-done", false],
    ["legenda.pivo", "--st-pivot", false],
    ["legenda.auxiliar", "--st-aux", true],
    ["legenda.cursores", "--accent", false],
  ];

  return (
    <section className="painel">
      <h2>{t("painel.legenda")}</h2>
      <ul className="legenda">
        {itens.map(([chave, token, tracejado]) => (
          <li key={chave}>
            <span
              className={tracejado ? "amostra tracejada" : "amostra"}
              style={{ background: `var(${token})` }}
              aria-hidden="true"
            />
            {t(chave)}
          </li>
        ))}
        <li>
          <span className="amostra apagada" aria-hidden="true" />
          {t("legenda.fora")}
        </li>
      </ul>
    </section>
  );
}

/** A fase vem como id de mensagem mais operandos — nunca como texto do C. */
function textoDaFase(fase: { str: number; a: number; b: number }): string {
  const chave = STR_CHAVES[fase.str];
  const nome = chave ? t(chave as Chave) : "";
  if (fase.b !== 0) return `${nome} ${fase.a}…${fase.b}`;
  return fase.a !== 0 ? `${nome} ${fase.a}` : nome;
}

function textoDoErro(codigo: number): string | null {
  if (codigo === Status.OK) return null;
  const chave = ERR_CHAVES[codigo];
  return chave ? t(chave as Chave) : String(codigo);
}
