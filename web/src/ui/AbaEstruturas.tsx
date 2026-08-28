/* A aba 1: as estruturas lineares, cada uma nas suas implementações.
 *
 * O React cuida do chrome — barra lateral, painéis, controles. Ele não sabe
 * que existe um canvas além de uma ref: o modelo e o desenho vivem no Player e
 * no GrafoView, fora do ciclo de renderização, e o React só recebe uma foto
 * com throttle.
 *
 * O cabeçalho e a troca de idioma saíram daqui quando a aba de ordenação
 * chegou: são da moldura, e não desta aba. */

import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  useSyncExternalStore,
} from "react";

import { chamar, selecionarSlot, sessaoNova, tamanho } from "../core/bridge";
import { Op, Percurso, Status, Tipo } from "../core/ops";
import { Player, type Operacao } from "../core/player";
import { ERR_CHAVES } from "../core/ops";
import { t, type Chave } from "../i18n";
import { ArvoreBView } from "../render/arvoreBView";
import { ArvoreBMaisView } from "../render/arvoreBMaisView";
import { ArvoreView } from "../render/arvoreView";
import { GrafoView } from "../render/grafoView";
import { HashView } from "../render/hashView";
import { ListaView } from "../render/listaView";
import { VetorView } from "../render/vetorView";
import {
  ESTRUTURAS,
  estruturaDe,
  parDaFamilia,
  type Estrutura,
} from "./Estruturas";
import { PainelCodigo } from "./PainelCodigo";
import { PainelScript } from "./PainelScript";
import { PainelLog, PainelMetricas } from "./PainelLateral";
import type { Passo } from "./Script";
import { Transporte } from "./Transporte";

/* Os três percursos, na ordem da aula. Uma tabela e não três botões escritos
 * à mão: acrescentar um percurso no C é acrescentar uma linha aqui. */
const PERCURSOS: Array<[number, Chave]> = [
  [Percurso.PERC_EM_ORDEM, "perc.emOrdem"],
  [Percurso.PERC_PRE_ORDEM, "perc.preOrdem"],
  [Percurso.PERC_POS_ORDEM, "perc.posOrdem"],
];

export function AbaEstruturas() {
  const player = useMemo(() => new Player(), []);
  const foto = useSyncExternalStore(player.assinar, player.ler);

  const [erro, setErro] = useState<string | null>(null);
  const [valor, setValor] = useState("42");
  const [tipo, setTipo] = useState<number>(ESTRUTURAS[0]!.tipo);
  const [capacidade, setCapacidade] = useState(8);
  const [comparar, setComparar] = useState(false);
  const [posicao, setPosicao] = useState("0");

  const estrutura = estruturaDe(tipo);

  /* As trilhas em cena: uma só, ou o par de implementações da família.
   *
   * Tudo o que vem depois — sessões, operações, canvas, painéis — percorre
   * esta lista. É o que evita um `if (comparando)` em cada um deles. */
  const trilhas: Estrutura[] = useMemo(
    () => (comparar ? parDaFamilia(estrutura.familia) : [estrutura]),
    [comparar, estrutura],
  );

  /* Quais trilhas ganham painel de código. Com duas, os dois lado a lado são
   * o argumento; com quatro, cada um viraria duas linhas. */
  const paineisDeCodigo = useMemo(() => {
    const todos = trilhas.map((faixa, k) => ({ faixa, k }));
    if (todos.length <= 2) return todos;
    const escolhido = todos.find((x) => x.faixa.tipo === tipo);
    return escolhido ? [escolhido] : todos.slice(0, 1);
  }, [trilhas, tipo]);

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

  /* Abrir a sessão na montagem, e de novo a cada troca de estrutura,
   * capacidade ou modo: o C não guarda histórico, e a linha do tempo do Player
   * também não faria sentido atravessando duas estruturas diferentes.
   *
   * Não há espera pelo núcleo aqui — a moldura só monta a aba depois de
   * `iniciar()` ter resolvido. */
  useEffect(() => {
    try {
      player.carregarTrilhas([abrirSessoes()]);
      setErro(null);
    } catch (e) {
      setErro(String(e));
    }
  }, [player, abrirSessoes]);

  /* ---- laço de animação ----------------------------------------------- */

  useEffect(() => player.iniciarLaco(), [player]);

  useEffect(() => {
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
      if (
        t_.mundo === "vetor" ||
        t_.mundo === "busca" ||
        t_.mundo === "hashAbe"
      ) {
        return new VetorView(canvas, t_.tipo);
      }
      if (t_.mundo === "hashEnc") return new HashView(canvas);
      if (t_.mundo === "lista") return new ListaView(canvas, t_.tipo);
      if (t_.mundo === "arvore") return new ArvoreView(canvas);
      if (t_.mundo === "arvoreB") return new ArvoreBView(canvas);
      if (t_.mundo === "arvoreBMais") return new ArvoreBMaisView(canvas);
      return new GrafoView(canvas, t_.tipo);
    });

    const solta = player.aoQuadro(() => {
      views.forEach((view, k) => view?.desenhar(player.estadoDe(k)));
    });

    return () => {
      solta();
      for (const view of views) view?.destruir();
    };
  }, [player, trilhas]);

  /* ---- operações ------------------------------------------------------ */

  /* Executa a mesma operação em cada trilha e devolve os eventos separados.
   *
   * É aqui que o modo comparar acontece: uma operação, dois slots, dois traces
   * — e o Player os alinha para as duas começarem juntas. O erro que interessa
   * é o da trilha 0; quando as duas implementações são do mesmo TAD elas
   * recusam pelo mesmo motivo, e quando divergem é a de vetor que enche
   * primeiro, o que o painel dela já mostra. */
  const executar = useCallback(
    (op: Op, a: number, b: number): { operacao: Operacao; erro: number } => {
      const operacao: Operacao = [];
      let erroDaPrimeira = Status.OK;

      trilhas.forEach((_, slot) => {
        selecionarSlot(slot);
        const saida = chamar(op, a, b);
        operacao.push(saida.eventos);
        if (slot === 0) erroDaPrimeira = saida.erro;
      });

      return { operacao, erro: erroDaPrimeira };
    },
    [trilhas],
  );

  const rodar = useCallback(
    (op: Op, a = 0, b = 0) => {
      const { operacao, erro: codigo } = executar(op, a, b);
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
        operacoes.push(executar(passo.op, passo.valor, passo.posicao).operacao);
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

  return (
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

          {trilhas.some((x) => x.posicoes) && (
            <div className="campo campo-capacidade">
              <label htmlFor="pos">{t("op.posicao")}</label>
              <input
                id="pos"
                className="mono"
                type="number"
                min={0}
                value={posicao}
                onChange={(ev) => setPosicao(ev.target.value)}
              />
            </div>
          )}

          {/* O mesmo campo serve às duas coisas, e o rótulo muda: numa
              estrutura com vetor ele é a capacidade; numa tabela hash ele é o
              `m`, e trocar 8 por 7 muda o desenho inteiro sem trocar uma linha
              de código — é metade da lição do hash. */}
          {trilhas.some(
            (x) =>
              x.mundo === "vetor" ||
              x.mundo === "hashEnc" ||
              x.mundo === "hashAbe" ||
              x.mundo === "arvoreB" ||
              x.mundo === "arvoreBMais",
          ) && (
            <div className="campo campo-capacidade">
              <label htmlFor="cap">
                {/* O mesmo campo, três significados: capacidade num vetor,
                    `m` numa tabela hash, e GRAU numa árvore B — que é o único
                    parâmetro que ela tem, e o que muda a forma inteira. */}
                {trilhas.some((x) => x.familia === "hash")
                  ? t("metrica.baldes")
                  : trilhas.some((x) => x.familia === "disco")
                    ? t("metrica.grau")
                    : t("op.capacidade")}
              </label>
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
                onClick={() => rodar(Op.OP_PUSH, Number(valor) | 0)}
            >
              {t(estrutura.rotuloInserir)}
            </button>
            {/* Remover e consultar SEM argumento não existem em toda
                estrutura: numa tabela hash não há "o primeiro" nem "o menor".
                O ponteiro nulo no vtable é a verdade, e a interface a espelha
                escondendo o botão — mostrar e recusar seria pior. */}
            {!estrutura.semExtremos && (
              <>
                <button
                  type="button"
                  className="secundario"
                  onClick={() => rodar(Op.OP_POP)}
                >
                  {t(estrutura.rotuloRemover)}
                </button>
                <button
                  type="button"
                  className="secundario"
                  onClick={() => rodar(Op.OP_TOPO)}
                >
                  {t(estrutura.rotuloConsultar)}
                </button>
              </>
            )}
            {estrutura.posicoes && (
              <>
                <button
                  type="button"
                  className="secundario"
                onClick={() =>
                    rodar(Op.OP_INSERIR_EM, Number(valor) | 0, Number(posicao) | 0)
                  }
                >
                  {t("op.inserirEm")}
                </button>
                <button
                  type="button"
                  className="secundario"
                onClick={() =>
                    /* O tamanho vem do C, não do modelo: o do modelo é o do
                       instante da animação, e inserir "no fim" enquanto a
                       animação anterior ainda roda cairia no meio. */
                    rodar(Op.OP_INSERIR_EM, Number(valor) | 0, tamanho())
                  }
                >
                  {t("op.inserirFim")}
                </button>
                <button
                  type="button"
                  className="secundario"
                onClick={() => rodar(Op.OP_REMOVER_EM, 0, Number(posicao) | 0)}
                >
                  {t("op.removerEm")}
                </button>
              </>
            )}

            {/* Remover por valor e percorrer são da árvore. Numa árvore a
                posição não existe, e é a remoção por valor que traz os três
                casos — o botão é a porta de entrada da aula inteira. */}
            {(estrutura.arvore || estrutura.porValor) && (
              <>
                <button
                  type="button"
                  className="secundario"
                  onClick={() => rodar(Op.OP_REMOVER_VALOR, Number(valor) | 0)}
                >
                  {t("op.removerValor")}
                </button>
                {/* Percurso é só de quem tem ordem. A tabela hash remove por
                    valor e não percorre — a ordem dela é acidente. */}
                {estrutura.arvore &&
                  PERCURSOS.map(([perc, chave]) => (
                    <button
                      key={perc}
                      type="button"
                      className="secundario"
                      onClick={() => rodar(Op.OP_PERCURSO, perc)}
                    >
                      {t(chave)}
                    </button>
                  ))}

                {/* As de disco percorrem por um botão só, e só em ordem. É a
                    operação que separa a árvore B da B+: a mesma leitura, e o
                    contador de leituras de disco contando histórias
                    diferentes. */}
                {estrutura.varredura && (
                  <button
                    type="button"
                    className="secundario"
                    onClick={() => rodar(Op.OP_PERCURSO, Percurso.PERC_EM_ORDEM)}
                  >
                    {t("op.varrer")}
                  </button>
                )}
              </>
            )}

            {/* Buscar não é operação de quem tem posição — é de quem tem
                `buscar` no vtable. A lista tem os dois; o vetor ordenado tem
                só este, porque nele a posição é consequência do valor. */}
            {estrutura.buscavel && (
              <button
                type="button"
                className="secundario"
                onClick={() => rodar(Op.OP_BUSCAR, Number(valor) | 0)}
              >
                {t("op.buscar")}
              </button>
            )}

            <button
              type="button"
              className="secundario"
                onClick={() => rodar(Op.OP_LIMPAR)}
            >
              {t("op.limpar")}
            </button>
            <button
              type="button"
              className="secundario"
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
              </section>

        <PainelScript desativado={false} aoRodar={rodarScript} />

        {trilhas.map((faixa, k) => (
          <PainelMetricas
            key={faixa.tipo}
            modelo={player.estadoDe(k)}
            mundo={faixa.mundo}
            i={foto.i}
            total={foto.total}
            titulo={trilhas.length > 1 ? t(faixa.nome) : undefined}
            rotativa={faixa.tipo === Tipo.TIPO_AVL}
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
        {/* Com mais de duas trilhas, cada painel de código caberia em duas
            linhas e deixaria de servir para o que existe: acompanhar a linha
            que está executando. Acima disso, só a estrutura selecionada — que
            é a que o seletor da esquerda continua apontando. */}
        {paineisDeCodigo.map(({ faixa, k }) => {
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
  );
}

/** O ERR_* vira frase pelo i18n; OK vira ausência de erro. */
function textoDoErro(codigo: number): string | null {
  if (codigo === Status.OK) return null;
  const chave = ERR_CHAVES[codigo];
  return chave ? t(chave as Chave) : String(codigo);
}
