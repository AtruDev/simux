/* web/src/core/player.ts — play, pause, passo, scrub e velocidade.
 *
 * O estado da animação vive aqui, fora do React. O React só recebe uma foto,
 * com throttle: redesenhar a árvore de componentes a 60 Hz para mexer um
 * contador é desperdício, e trava a animação em máquina modesta.
 *
 * Cada ds_call zera o trace do lado do C. A linha do tempo que dá para
 * arrastar é acumulada deste lado: é ela que deixa voltar cinco operações. */

import { aplicar } from "../model/aplicar";
import { modeloNovo, type Modelo } from "../model/modelo";
import type { Ev } from "./bridge";

/** Eventos por segundo em velocidade 1x. */
const RITMO_BASE = 6;

/** Frequência com que o React é avisado. Não 60. */
const HZ_REACT = 15;

export interface FotoPlayer {
  readonly i: number;
  readonly total: number;
  readonly tocando: boolean;
  readonly velocidade: number;
}

export class Player {
  private eventos: Ev[] = [];
  private modelo: Modelo = modeloNovo();
  private i = 0;
  private vel = 1;
  private tocando = false;

  private acumulado = 0;
  private instanteAnterior = 0;
  private raf = 0;

  private assinantes = new Set<() => void>();
  private porQuadro = new Set<() => void>();
  private avisoPendente = 0;
  private foto: FotoPlayer = { i: 0, total: 0, tocando: false, velocidade: 1 };

  constructor() {
    this.laco = this.laco.bind(this);
  }

  iniciarLaco(): () => void {
    if (this.raf === 0) {
      this.instanteAnterior = performance.now();
      this.raf = requestAnimationFrame(this.laco);
    }
    return () => {
      cancelAnimationFrame(this.raf);
      this.raf = 0;
    };
  }

  /* ---- leitura ------------------------------------------------------- */

  get estado(): Modelo {
    return this.modelo;
  }

  get eventoAtual(): Ev | null {
    return this.i > 0 ? (this.eventos[this.i - 1] ?? null) : null;
  }

  /** Eventos já aplicados, do mais recente para o mais antigo. */
  historico(limite: number): Ev[] {
    const inicio = Math.max(0, this.i - limite);
    return this.eventos.slice(inicio, this.i).reverse();
  }

  ler = (): FotoPlayer => this.foto;

  assinar = (fn: () => void): (() => void) => {
    this.assinantes.add(fn);
    return () => this.assinantes.delete(fn);
  };

  /** Chamado a cada quadro. É por aqui que o renderizador desenha. */
  aoQuadro(fn: () => void): () => void {
    this.porQuadro.add(fn);
    return () => this.porQuadro.delete(fn);
  }

  /* ---- linha do tempo ------------------------------------------------- */

  /** Troca a linha do tempo inteira e volta ao começo. */
  carregar(evs: Ev[]): void {
    this.eventos = evs.slice();
    this.pause();
    this.irPara(0);
  }

  /** Acrescenta o trace de uma operação nova e a mostra acontecendo.
   *
   * A linha do tempo nunca é truncada. A estrutura que o C tem reflete todas
   * as operações já executadas, esteja o cursor onde estiver — onde a
   * animação parou é assunto da vista. Descartar eventos aqui apagaria
   * operações que de fato aconteceram, e foi exatamente o que acontecia ao
   * operar antes de a animação anterior terminar. */
  anexar(evs: Ev[]): void {
    const inicioDosNovos = this.eventos.length;
    this.eventos.push(...evs);

    /* Volta ao instante anterior aos eventos novos, para eles serem vistos
     * acontecendo em vez de aparecerem prontos. */
    this.irPara(inicioDosNovos);
    this.tocando = evs.length > 0;
    this.avisar();
  }

  /** Vai ao passo k. Voltar reexecuta de 0 — nunca desfaz evento.
   *
   * Ir para a frente é só continuar aplicando, porque o modelo já é
   * exatamente o estado do passo atual. Voltar não tem esse atalho: é aí
   * que a reexecução paga o seu preço, e é o que dispensa escrever o
   * inverso de cada evento. */
  irPara(k: number): void {
    const destino = Math.max(0, Math.min(k, this.eventos.length));

    if (destino < this.i) {
      this.modelo = modeloNovo();
      for (let j = 0; j < destino; j++) {
        const ev = this.eventos[j];
        if (ev) aplicar(this.modelo, ev);
      }
    } else {
      for (let j = this.i; j < destino; j++) {
        const ev = this.eventos[j];
        if (ev) aplicar(this.modelo, ev);
      }
    }

    this.i = destino;
    this.acumulado = 0;
    this.avisar();
  }

  passo(delta: number): void {
    this.pause();
    this.irPara(this.i + delta);
  }

  play(): void {
    if (this.i >= this.eventos.length) {
      this.irPara(0);
    }
    this.tocando = true;
    this.acumulado = 0;
    this.avisar();
  }

  pause(): void {
    if (this.tocando) {
      this.tocando = false;
      this.avisar();
    }
  }

  alternar(): void {
    if (this.tocando) this.pause();
    else this.play();
  }

  setVelocidade(v: number): void {
    this.vel = v;
    this.avisar();
  }

  /* ---- laço ----------------------------------------------------------- */

  private laco(agora: number): void {
    const dt = Math.min((agora - this.instanteAnterior) / 1000, 0.25);
    this.instanteAnterior = agora;

    if (this.tocando) {
      this.acumulado += dt * RITMO_BASE * this.vel;
      while (this.acumulado >= 1 && this.i < this.eventos.length) {
        const ev = this.eventos[this.i];
        if (ev) aplicar(this.modelo, ev);
        this.i++;
        this.acumulado -= 1;
      }
      if (this.i >= this.eventos.length) {
        this.tocando = false;
        this.acumulado = 0;
      }
      this.avisar();
    }

    for (const fn of this.porQuadro) fn();

    this.raf = requestAnimationFrame(this.laco);
  }

  private avisar(): void {
    const mudou =
      this.foto.i !== this.i ||
      this.foto.total !== this.eventos.length ||
      this.foto.tocando !== this.tocando ||
      this.foto.velocidade !== this.vel;

    if (!mudou) return;

    /* A foto é sempre atualizada. Antes ela ficava presa junto com o aviso, e
     * quem lesse logo depois de uma operação via dados velhos — o throttle é
     * sobre com que frequência o React redesenha, não sobre a verdade. */
    this.foto = {
      i: this.i,
      total: this.eventos.length,
      tocando: this.tocando,
      velocidade: this.vel,
    };

    /* Durante o play o índice muda várias vezes por segundo, e o painel não
     * precisa acompanhar quadro a quadro. Nenhum aviso se perde: o laço chama
     * isto a cada quadro, e o fim da reprodução zera `tocando`, que passa
     * direto pelo throttle. */
    const agora = performance.now();
    if (this.tocando && agora - this.avisoPendente < 1000 / HZ_REACT) return;
    this.avisoPendente = agora;

    for (const fn of this.assinantes) fn();
  }
}
