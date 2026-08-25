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

import { ErroDs, exec, iniciar, sessaoNova } from "./core/bridge";
import { Op } from "./core/ops";
import { Player } from "./core/player";
import { ERR_CHAVES } from "./core/ops";
import { definirIdioma, idiomaAtual, t, type Chave, type Idioma } from "./i18n";
import { GrafoView } from "./render/grafoView";
import { VetorView } from "./render/vetorView";
import { ESTRUTURAS, estruturaDe } from "./ui/Estruturas";
import { PainelCodigo } from "./ui/PainelCodigo";
import { PainelLog, PainelMetricas } from "./ui/PainelLateral";
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

  const estrutura = estruturaDe(tipo);

  const refCanvas = useRef<HTMLCanvasElement>(null);

  /* ---- núcleo --------------------------------------------------------- */

  useEffect(() => {
    let vivo = true;
    iniciar()
      .then(() => {
        if (!vivo) return;
        player.carregar(sessaoNova(tipo, capacidade));
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

  /* Trocar estrutura ou capacidade descarta a sessão e começa outra. O C não
   * guarda histórico, e a linha do tempo do Player também não faria sentido
   * atravessando duas estruturas diferentes. */
  useEffect(() => {
    if (!carregado) return;
    try {
      player.carregar(sessaoNova(tipo, capacidade));
      setErro(null);
    } catch (e) {
      setErro(String(e));
    }
  }, [player, carregado, tipo, capacidade]);

  /* ---- laço de animação ----------------------------------------------- */

  useEffect(() => player.iniciarLaco(), [player]);

  useEffect(() => {
    const canvas = refCanvas.current;
    if (!canvas || !carregado) return;

    /* O renderizador é escolhido pelo mundo da estrutura, não pela estrutura:
     * pilha e fila com vetor desenham no mesmo VetorView, e é o rótulo do
     * ponteiro que muda. */
    const view =
      estrutura.mundo === "vetor"
        ? new VetorView(canvas, tipo)
        : new GrafoView(canvas);

    const solta = player.aoQuadro(() => view.desenhar(player));
    return () => {
      solta();
      view.destruir();
    };
  }, [player, carregado, tipo, estrutura.mundo]);

  /* ---- operações ------------------------------------------------------ */

  const rodar = useCallback(
    (op: Op, a = 0) => {
      try {
        player.anexar(exec(op, a));
        setErro(null);
      } catch (e) {
        if (e instanceof ErroDs) {
          const chave = ERR_CHAVES[e.codigo];
          setErro(chave ? t(chave as Chave) : String(e.codigo));
        } else {
          setErro(String(e));
        }
      }
    },
    [player],
  );

  const reiniciar = useCallback(() => {
    player.carregar(sessaoNova(tipo, capacidade));
    setErro(null);
  }, [player, tipo, capacidade]);

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

  const modelo = player.estado;
  const fonte = modelo.fonte;

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
                <label key={e.tipo} className="opcao">
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

            {estrutura.mundo === "vetor" && (
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

          <PainelMetricas modelo={modelo} i={foto.i} total={foto.total} />
        </aside>

        <main className="coluna centro">
          <canvas ref={refCanvas} className="canvas" />
          <Transporte
            player={player}
            i={foto.i}
            total={foto.total}
            tocando={foto.tocando}
            velocidade={foto.velocidade}
          />
        </main>

        <aside className="coluna direita">
          <PainelCodigo src={fonte?.src ?? 0} linha={fonte?.linha ?? 0} />
          <PainelLog eventos={player.historico(12)} mundo={estrutura.mundo} />
        </aside>
      </div>
    </div>
  );
}
