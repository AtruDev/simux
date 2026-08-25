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

  /** Acrescenta o trace de uma operação nova e toca a partir daqui. */
  anexar(evs: Ev[]): void {
    /* Se o usuário tinha voltado no tempo, a operação nova continua de onde
     * ele está: o que estava à frente nunca aconteceu, do ponto de vista
     * da estrutura que o C tem agora. */
    this.eventos.length = this.i;
    this.eventos.push(...evs);
    this.acumulado = 0;
    this.tocando = evs.length > 0;
    this.avisar();
  }

  /** Vai ao passo k reexecutando de 0 — nunca desfazendo evento. */
  irPara(k: number): void {
    const destino = Math.max(0, Math.min(k, this.eventos.length));
    this.modelo = modeloNovo();
    for (let j = 0; j < destino; j++) {
      const ev = this.eventos[j];
      if (ev) aplicar(this.modelo, ev);
    }
    this.i = destino;
    this.acumulado = 0;
    this.avisar();
  }

  passo(delta: number): void {
    this.pause();
    const destino = this.i + delta;
    if (delta > 0 && destino <= this.eventos.length) {
      /* Andar para a frente é só aplicar: não precisa reexecutar tudo. */
      for (let j = this.i; j < destino; j++) {
        const ev = this.eventos[j];
        if (ev) aplicar(this.modelo, ev);
      }
      this.i = destino;
      this.acumulado = 0;
      this.avisar();
    } else {
      this.irPara(destino);
    }
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
    const agora = performance.now();
    const mudouAlgoQueOReactPrecisa =
      this.foto.i !== this.i ||
      this.foto.total !== this.eventos.length ||
      this.foto.tocando !== this.tocando ||
      this.foto.velocidade !== this.vel;

    if (!mudouAlgoQueOReactPrecisa) return;

    /* Throttle: durante o play, o índice muda várias vezes por segundo, mas o
     * painel não precisa acompanhar quadro a quadro. */
    if (agora - this.avisoPendente < 1000 / HZ_REACT && this.tocando) return;
    this.avisoPendente = agora;

    this.foto = {
      i: this.i,
      total: this.eventos.length,
      tocando: this.tocando,
      velocidade: this.vel,
    };
    for (const fn of this.assinantes) fn();
  }
}
