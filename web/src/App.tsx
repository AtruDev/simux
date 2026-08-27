/* A tela das estruturas lineares: pilha e fila, cada uma nas duas
 * implementações.
 *
 * O React cuida do chrome — barra lateral, painéis, controles. Ele não sabe
 * que existe um canvas além de uma ref: o modelo e o desenho vivem no Player e
 * no GrafoView, fora do ciclo de renderização, e o React só recebe uma foto
 * com throttle. */

import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  useSyncExternalStore,
} from "react";

import { chamar, iniciar, selecionarSlot, sessaoNova } from "./core/bridge";
import { Op, Status } from "./core/ops";
import { Player, type Operacao } from "./core/player";
import { ERR_CHAVES } from "./core/ops";
import { definirIdioma, idiomaAtual, t, type Chave, type Idioma } from "./i18n";
import { GrafoView } from "./render/grafoView";
import { VetorView } from "./render/vetorView";
import {
  ESTRUTURAS,
  estruturaDe,
  parDaFamilia,
  type Estrutura,
} from "./ui/Estruturas";
import { PainelCodigo } from "./ui/PainelCodigo";
import { PainelScript } from "./ui/PainelScript";
import { PainelLog, PainelMetricas } from "./ui/PainelLateral";
import type { Passo } from "./ui/Script";
import { Transporte } from "./ui/Transporte";

export function App() {
  const player = useMemo(() => new Player(), []);
  const foto = useSyncExternalStore(player.assinar, player.ler);

  const [idioma, setIdioma] = useState<Idioma>(idiomaAtual());
  const [carregado, setCarregado] = useState(false);
  const [erro, setErro] = useState<string | null>(null);
  const [valor, setValor] = useState("42");
  const [tipo, setTipo] = useState<number>(ESTRUTURAS[0]!.tipo);
  const [capacidade, setCapacidade] = useState(8);
  const [comparar, setComparar] = useState(false);

  const estrutura = estruturaDe(tipo);

  /* As trilhas em cena: uma só, ou o par de implementações da família.
   *
   * Tudo o que vem depois — sessões, operações, canvas, painéis — percorre
   * esta lista. É o que evita um `if (comparando)` em cada um deles. */
  const trilhas: Estrutura[] = useMemo(
    () => (comparar ? parDaFamilia(estrutura.familia) : [estrutura]),
    [comparar, estrutura],
  );

  const refCanvas = useRef<Array<HTMLCanvasElement | null>>([]);

  /* ---- núcleo --------------------------------------------------------- */

  /* Abre uma sessão por trilha, cada uma no seu slot do core, e devolve os
   * eventos de criação já separados por trilha. */
  const abrirSessoes = useCallback((): Operacao => {
    return trilhas.map((t_, slot) => {
      selecionarSlot(slot);
      return sessaoNova(t_.tipo, capacidade);
    });
  }, [trilhas, capacidade]);

  useEffect(() => {
    let vivo = true;
    iniciar()
      .then(() => {
        if (!vivo) return;
        player.carregarTrilhas([abrirSessoes()]);
        setCarregado(true);
      })
      .catch((e: unknown) => vivo && setErro(String(e)));
    return () => {
      vivo = false;
    };
    /* Trocar de estrutura reabre a sessão: é o efeito abaixo que cuida disso,
     * e repetir a dependência aqui abriria duas. */
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [player]);

  /* Trocar estrutura, capacidade ou modo descarta as sessões e começa outras.
   * O C não guarda histórico, e a linha do tempo do Player também não faria
   * sentido atravessando duas estruturas diferentes. */
  useEffect(() => {
    if (!carregado) return;
    try {
      player.carregarTrilhas([abrirSessoes()]);
      setErro(null);
    } catch (e) {
      setErro(String(e));
    }
  }, [player, carregado, abrirSessoes]);

  /* ---- laço de animação ----------------------------------------------- */

  useEffect(() => player.iniciarLaco(), [player]);

  useEffect(() => {
    if (!carregado) return;

    /* Uma view por trilha, cada uma no seu canvas.
     *
     * Dois canvas, e não um dividido em duas faixas: assim cada view continua
     * achando que a superfície é toda dela, e o CSS resolve a divisão. Ensinar
     * as views a desenhar num retângulo seria mudança em duas delas para
     * ganhar nada.
     *
     * O renderizador é escolhido pelo mundo da estrutura, não pela estrutura:
     * pilha e fila com vetor desenham no mesmo VetorView, e é o rótulo do
     * ponteiro que muda. */
    const views = trilhas.map((t_, k) => {
      const canvas = refCanvas.current[k];
      if (!canvas) return null;
      return t_.mundo === "vetor"
        ? new VetorView(canvas, t_.tipo)
        : new GrafoView(canvas, t_.tipo);
    });

    const solta = player.aoQuadro(() => {
      views.forEach((view, k) => view?.desenhar(player.estadoDe(k)));
    });

    return () => {
      solta();
      for (const view of views) view?.destruir();
    };
  }, [player, carregado, trilhas]);

  /* ---- operações ------------------------------------------------------ */

  /* Executa a mesma operação em cada trilha e devolve os eventos separados.
   *
   * É aqui que o modo comparar acontece: uma operação, dois slots, dois traces
   * — e o Player os alinha para as duas começarem juntas. O erro que interessa
   * é o da trilha 0; quando as duas implementações são do mesmo TAD elas
   * recusam pelo mesmo motivo, e quando divergem é a de vetor que enche
   * primeiro, o que o painel dela já mostra. */
  const executar = useCallback(
    (op: Op, a: number): { operacao: Operacao; erro: number } => {
      const operacao: Operacao = [];
      let erroDaPrimeira = Status.OK;

      trilhas.forEach((_, slot) => {
        selecionarSlot(slot);
        const saida = chamar(op, a);
        operacao.push(saida.eventos);
        if (slot === 0) erroDaPrimeira = saida.erro;
      });

      return { operacao, erro: erroDaPrimeira };
    },
    [trilhas],
  );

  const rodar = useCallback(
    (op: Op, a = 0) => {
      const { operacao, erro: codigo } = executar(op, a);
      player.anexarTrilhas([operacao]);
      setErro(textoDoErro(codigo));
    },
    [player, executar],
  );

  /* Um script é uma anexação só, não uma por passo.
   *
   * Anexar por passo faria cada chamada saltar o cursor para o começo dos seus
   * próprios eventos, e o que se veria seria o último passo tocando sozinho.
   * Juntando tudo, o script inteiro toca do começo, e o transporte arrasta por
   * cima dele como se fosse uma operação longa. */
  const rodarScript = useCallback(
    (passos: Passo[]) => {
      const operacoes: Operacao[] = [];

      for (const passo of passos) {
        /* Uma operação recusada no meio do script não interrompe o resto: a
         * estrutura fica intacta, e a recusa é justamente o que o exercício
         * costuma querer mostrar. */
        operacoes.push(executar(passo.op, passo.valor).operacao);
      }

      /* E ela não vai para o aviso de erro no alto.
       *
       * O C executa o script inteiro na hora; a animação só começa depois. Pôr
       * a recusa no aviso faria "estrutura vazia" aparecer enquanto a tela
       * ainda mostra a pilha cheia — um erro sobre um instante que ainda não
       * chegou. Ela chega sozinha, no seu momento: o EV_MSG está no trace, sai
       * no log e ilumina a linha do .c que recusou. */
      player.anexarTrilhas(operacoes);
      setErro(null);
    },
    [player, executar],
  );

  const reiniciar = useCallback(() => {
    player.carregarTrilhas([abrirSessoes()]);
    setErro(null);
  }, [player, abrirSessoes]);

  /* ---- atalhos de teclado --------------------------------------------- */

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
        case "r":
        case "R":
          player.irPara(0);
          break;
        default:
          break;
      }
    }
    window.addEventListener("keydown", aoTeclar);
    return () => window.removeEventListener("keydown", aoTeclar);
  }, [player]);

  /* ---- render --------------------------------------------------------- */

  function alternarIdioma() {
    const novo: Idioma = idioma === "pt" ? "en" : "pt";
    definirIdioma(novo);
    setIdioma(novo);
  }

  return (
    <div className="app">
      <header className="cabecalho">
        <h1>{t("app.titulo")}</h1>
        <p className="descricao">{t("app.descricao")}</p>
        <button type="button" onClick={alternarIdioma} className="secundario">
          {t("app.trocarIdioma")}
        </button>
      </header>

      <div className="grade">
        <aside className="coluna esquerda">
          <section className="painel">
            <h2>{t("estrutura.titulo")}</h2>
            <div className="lista-estruturas">
              {ESTRUTURAS.map((e) => (
                <label
                  key={e.tipo}
                  className={
                    /* No modo comparar as duas implementações da família estão
                     * em cena, então as duas ficam marcadas. */
                    trilhas.some((x) => x.tipo === e.tipo)
                      ? "opcao em-cena"
                      : "opcao"
                  }
                >
                  <input
                    type="radio"
                    name="estrutura"
                    checked={tipo === e.tipo}
                    onChange={() => setTipo(e.tipo)}
                  />
                  <span>{t(e.nome)}</span>
                </label>
              ))}
            </div>

            <label className="opcao opcao-comparar">
              <input
                type="checkbox"
                checked={comparar}
                onChange={(ev) => setComparar(ev.target.checked)}
              />
              <span>{t("estrutura.comparar")}</span>
            </label>

            {trilhas.some((x) => x.mundo === "vetor") && (
              <div className="campo campo-capacidade">
                <label htmlFor="cap">{t("op.capacidade")}</label>
                <input
                  id="cap"
                  className="mono"
                  type="number"
                  min={1}
                  max={32}
                  value={capacidade}
                  onChange={(ev) => {
                    const n = Number(ev.target.value);
                    if (n >= 1 && n <= 32) setCapacidade(n);
                  }}
                />
              </div>
            )}
          </section>

          <section className="painel">
            <h2>{t("painel.operacoes")}</h2>

            <div className="campo">
              <label htmlFor="valor">{t("op.valor")}</label>
              <input
                id="valor"
                className="mono"
                value={valor}
                inputMode="numeric"
                onChange={(e) => setValor(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === "Enter") rodar(Op.OP_PUSH, Number(valor) | 0);
                }}
              />
              <button
                type="button"
                className="secundario"
                title={t("op.aleatorio")}
                onClick={() => setValor(String(Math.floor(Math.random() * 100)))}
              >
                ⚄
              </button>
            </div>

            <div className="operacoes">
              <button
                type="button"
                disabled={!carregado}
                onClick={() => rodar(Op.OP_PUSH, Number(valor) | 0)}
              >
                {t(estrutura.rotuloInserir)}
              </button>
              <button
                type="button"
                className="secundario"
                disabled={!carregado}
                onClick={() => rodar(Op.OP_POP)}
              >
                {t(estrutura.rotuloRemover)}
              </button>
              <button
                type="button"
                className="secundario"
                disabled={!carregado}
                onClick={() => rodar(Op.OP_TOPO)}
              >
                {t(estrutura.rotuloConsultar)}
              </button>
              <button
                type="button"
                className="secundario"
                disabled={!carregado}
                onClick={() => rodar(Op.OP_LIMPAR)}
              >
                {t("op.limpar")}
              </button>
              <button
                type="button"
                className="secundario"
                disabled={!carregado}
                onClick={reiniciar}
              >
                ⟲
              </button>
            </div>

            {erro !== null && (
              <p className="erro">
                {t("app.erro")}: {erro}
              </p>
            )}
            {!carregado && <p className="vazio">{t("app.carregando")}</p>}
          </section>

          <PainelScript desativado={!carregado} aoRodar={rodarScript} />

          {trilhas.map((faixa, k) => (
            <PainelMetricas
              key={faixa.tipo}
              modelo={player.estadoDe(k)}
              i={foto.i}
              total={foto.total}
              titulo={trilhas.length > 1 ? t(faixa.nome) : undefined}
            />
          ))}
        </aside>

        <main className="coluna centro">
          <div className="faixas">
            {trilhas.map((faixa, k) => (
              <div className="faixa" key={faixa.tipo}>
                {trilhas.length > 1 && (
                  <span className="faixa-nome">{t(faixa.nome)}</span>
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
        </main>

        <aside className="coluna direita">
          {trilhas.map((faixa, k) => {
            const daTrilha = player.estadoDe(k).fonte;
            return (
              <PainelCodigo
                key={faixa.tipo}
                src={daTrilha?.src ?? 0}
                linha={daTrilha?.linha ?? 0}
                titulo={trilhas.length > 1 ? t(faixa.nome) : undefined}
              />
            );
          })}

          {/* O log fica de fora do modo comparar: dois logs lado a lado
              cansam mais do que informam, e o espaço vale mais para o segundo
              painel de código, que é onde a comparação fica evidente — os dois
              arquivos executando a mesma operação, cada um na sua linha. */}
          {trilhas.length === 1 && (
            <PainelLog eventos={player.historico(12)} mundo={estrutura.mundo} />
          )}
        </aside>
      </div>
    </div>
  );
}

/** O ERR_* vira frase pelo i18n; OK vira ausência de erro. */
function textoDoErro(codigo: number): string | null {
  if (codigo === Status.OK) return null;
  const chave = ERR_CHAVES[codigo];
  return chave ? t(chave as Chave) : String(codigo);
}
