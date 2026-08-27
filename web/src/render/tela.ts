/* web/src/render/tela.ts — o que todo renderizador de nós repete.
 *
 * Canvas com devicePixelRatio, paleta lida dos tokens, prefers-reduced-motion,
 * o mapa de poses e o tween. Nada aqui sabe se a estrutura cresce para baixo
 * ou para o lado: isso é assunto de quem herda.
 *
 * Existe porque a lista quis um layout horizontal com quebra de linha, e pôr
 * as duas orientações no mesmo arquivo teria dado um `if` em cada método. */

import { lerPaleta, type Paleta } from "./tokens";

/** Posição atual e alvo de um nó. O tween aproxima uma da outra. */
export interface Pose {
  x: number;
  y: number;
  alvoX: number;
  alvoY: number;
  alfa: number;
  alvoAlfa: number;
}

/** atual += (alvo - atual) * isto, por quadro. */
export const SUAVIZACAO = 0.2;

export abstract class Tela {
  protected ctx: CanvasRenderingContext2D;
  protected paleta: Paleta;
  protected poses = new Map<number, Pose>();
  protected larguraCss = 0;
  protected alturaCss = 0;
  protected semMovimento: boolean;
  private observador: ResizeObserver;

  constructor(protected canvas: HTMLCanvasElement) {
    const ctx = canvas.getContext("2d");
    if (!ctx) throw new Error("canvas 2d indisponível");
    this.ctx = ctx;
    this.paleta = lerPaleta(document.documentElement);
    this.semMovimento = window.matchMedia(
      "(prefers-reduced-motion: reduce)",
    ).matches;

    this.observador = new ResizeObserver(() => this.redimensionar());
    this.observador.observe(canvas);
    this.redimensionar();
  }

  destruir(): void {
    this.observador.disconnect();
  }

  /** Em tela de alta densidade, sem isto o desenho sai borrado. */
  private redimensionar(): void {
    const dpr = window.devicePixelRatio || 1;
    const r = this.canvas.getBoundingClientRect();
    this.larguraCss = r.width;
    this.alturaCss = r.height;
    this.canvas.width = Math.round(r.width * dpr);
    this.canvas.height = Math.round(r.height * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  protected get passo(): number {
    return this.semMovimento ? 1 : SUAVIZACAO;
  }

  /** Cria a pose de um nó novo, ou atualiza o alvo de um que já existe. */
  protected mirar(id: number, alvoX: number, alvoY: number, nascer: Pose): void {
    let pose = this.poses.get(id);
    if (!pose) {
      pose = nascer;
      this.poses.set(id, pose);
    }
    pose.alvoX = alvoX;
    pose.alvoY = alvoY;
    pose.alvoAlfa = 1;
  }

  /** Some com quem saiu do modelo e aproxima todo mundo do alvo. */
  protected animar(vivos: (id: number) => boolean): void {
    const k = this.passo;

    /* A pose só é descartada quando o nó terminou de sumir: apagá-la junto
     * com o evento faria o nó desaparecer de um quadro para o outro. */
    for (const [id, pose] of this.poses) {
      if (!vivos(id)) {
        pose.alvoAlfa = 0;
        if (pose.alfa < 0.02) this.poses.delete(id);
      }
    }

    for (const pose of this.poses.values()) {
      pose.x += (pose.alvoX - pose.x) * k;
      pose.y += (pose.alvoY - pose.y) * k;
      pose.alfa += (pose.alvoAlfa - pose.alfa) * k;
    }
  }

  protected limpar(): void {
    this.ctx.clearRect(0, 0, this.larguraCss, this.alturaCss);
  }

  protected retangulo(x: number, y: number, w: number, h: number, r: number) {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.arcTo(x + w, y, x + w, y + h, r);
    ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r);
    ctx.arcTo(x, y, x + w, y, r);
    ctx.closePath();
  }

  protected setaBaixo(x: number, y: number, cor: string) {
    const ctx = this.ctx;
    ctx.fillStyle = cor;
    ctx.beginPath();
    ctx.moveTo(x, y + 2);
    ctx.lineTo(x - 5, y - 6);
    ctx.lineTo(x + 5, y - 6);
    ctx.closePath();
    ctx.fill();
  }

  protected setaDireita(x: number, y: number, cor: string) {
    const ctx = this.ctx;
    ctx.fillStyle = cor;
    ctx.beginPath();
    ctx.moveTo(x + 2, y);
    ctx.lineTo(x - 6, y - 5);
    ctx.lineTo(x - 6, y + 5);
    ctx.closePath();
    ctx.fill();
  }

  protected setaEsquerda(x: number, y: number, cor: string) {
    const ctx = this.ctx;
    ctx.fillStyle = cor;
    ctx.beginPath();
    ctx.moveTo(x - 2, y);
    ctx.lineTo(x + 6, y - 5);
    ctx.lineTo(x + 6, y + 5);
    ctx.closePath();
    ctx.fill();
  }
}
