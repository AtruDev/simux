/* web/src/render/grafoView.ts — nós, setas e ponteiros nomeados no canvas.
 *
 * Duas responsabilidades separadas, como manda o plano: o layout calcula a
 * posição ALVO de cada nó, e o tween aproxima a posição atual do alvo a cada
 * quadro. É essa separação que faz um pop parecer um pop sem existir uma única
 * linha de código de animação de pop. */

import { Ptr } from "../core/ops";
import type { Player } from "../core/player";
import { alvoDe, type Modelo } from "../model/modelo";
import { lerPaleta, type Paleta } from "./tokens";

const LARGURA_NO = 118;
const ALTURA_NO = 46;
const ESPACO = 34; /* vão entre um nó e o seguinte */
const MARGEM_TOPO = 52;
const RAIO = 8;
const SUAVIZACAO = 0.2; /* atual += (alvo - atual) * isto, por quadro */

interface Pose {
  x: number;
  y: number;
  alvoX: number;
  alvoY: number;
  alfa: number;
  alvoAlfa: number;
}

export class GrafoView {
  private ctx: CanvasRenderingContext2D;
  private paleta: Paleta;
  private poses = new Map<number, Pose>();
  private larguraCss = 0;
  private alturaCss = 0;
  private observador: ResizeObserver;
  private semMovimento: boolean;

  constructor(private canvas: HTMLCanvasElement) {
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

  /* ---- layout --------------------------------------------------------- */

  /** A cadeia a partir do topo, seguindo o slot 0 de cada nó. */
  private cadeia(m: Modelo): number[] {
    const fila: number[] = [];
    const visto = new Set<number>();
    let id = alvoDe(m, Ptr.PTR_TOPO);

    while (id !== 0 && m.nos.has(id) && !visto.has(id)) {
      visto.add(id);
      fila.push(id);
      id = m.nos.get(id)?.arestas.get(0) ?? 0;
    }

    /* Um nó fora da cadeia é sinal de bug no trace. Desenhá-lo mesmo assim é
     * melhor que escondê-lo: sumir sem explicação seria pior de depurar. */
    for (const orfao of m.ordem) {
      if (!visto.has(orfao)) fila.push(orfao);
    }
    return fila;
  }

  /* ---- quadro --------------------------------------------------------- */

  desenhar(player: Player): void {
    const m = player.estado;
    const ordem = this.cadeia(m);
    const cx = this.larguraCss / 2;
    const k = this.semMovimento ? 1 : SUAVIZACAO;

    ordem.forEach((id, indice) => {
      const alvoY = MARGEM_TOPO + indice * (ALTURA_NO + ESPACO);
      let pose = this.poses.get(id);
      if (!pose) {
        /* Nasce um pouco acima e transparente: o push entra deslizando de
         * cima, que é de onde a pilha cresce. */
        pose = { x: cx, y: alvoY - 28, alvoX: cx, alvoY, alfa: 0, alvoAlfa: 1 };
        this.poses.set(id, pose);
      }
      pose.alvoX = cx;
      pose.alvoY = alvoY;
      pose.alvoAlfa = 1;
    });

    /* Quem saiu do modelo desaparece; a pose só é descartada quando some. */
    for (const [id, pose] of this.poses) {
      if (!m.nos.has(id)) {
        pose.alvoAlfa = 0;
        if (pose.alfa < 0.02) this.poses.delete(id);
      }
    }

    for (const pose of this.poses.values()) {
      pose.x += (pose.alvoX - pose.x) * k;
      pose.y += (pose.alvoY - pose.y) * k;
      pose.alfa += (pose.alvoAlfa - pose.alfa) * k;
    }

    this.pintar(m, ordem);
  }

  private pintar(m: Modelo, ordem: number[]): void {
    const ctx = this.ctx;
    const p = this.paleta;

    ctx.clearRect(0, 0, this.larguraCss, this.alturaCss);

    /* Arestas primeiro, para o nó cobrir a ponta da seta. */
    for (const id of ordem) {
      const pose = this.poses.get(id);
      if (!pose) continue;

      const destino = m.nos.get(id)?.arestas.get(0) ?? 0;
      const poseDestino = destino !== 0 ? this.poses.get(destino) : undefined;
      const y0 = pose.y + ALTURA_NO / 2;
      const y1 = poseDestino ? poseDestino.y - ALTURA_NO / 2 : y0 + 24;

      ctx.globalAlpha = pose.alfa;
      ctx.strokeStyle = p.linha2;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(pose.x, y0);
      ctx.lineTo(pose.x, y1);
      ctx.stroke();

      if (poseDestino) {
        this.setaBaixo(pose.x, y1, p.linha2);
      } else {
        /* Fim da cadeia: o NULL que o C manda como 0. */
        ctx.fillStyle = p.fg3;
        ctx.font = `500 12px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "top";
        ctx.fillText("NULL", pose.x, y1 + 4);
      }
      ctx.globalAlpha = 1;
    }

    for (const id of ordem) {
      const pose = this.poses.get(id);
      const no = m.nos.get(id);
      if (!pose || !no) continue;

      const visitado = m.visitados.has(id);
      ctx.globalAlpha = pose.alfa;

      this.retangulo(
        pose.x - LARGURA_NO / 2,
        pose.y - ALTURA_NO / 2,
        LARGURA_NO,
        ALTURA_NO,
        RAIO,
      );
      ctx.fillStyle = p.bg2;
      ctx.fill();

      /* O acento é da interface, e o cursor é justamente onde a interface está
       * olhando — é a única vez que ele encosta num nó. */
      ctx.strokeStyle = visitado ? p.acento : p.linha2;
      ctx.lineWidth = visitado ? 2 : 1;
      ctx.stroke();

      ctx.fillStyle = p.fg;
      ctx.font = `600 17px ${p.mono}`;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(String(no.valor), pose.x - 14, pose.y);

      /* O id liga o desenho ao trace. Discreto, mas é o que salva na hora de
       * depurar uma animação errada. */
      ctx.fillStyle = p.fg3;
      ctx.font = `500 11px ${p.mono}`;
      ctx.fillText(`#${id}`, pose.x + 38, pose.y);

      ctx.globalAlpha = 1;
    }

    this.ponteiro(m, "topo", Ptr.PTR_TOPO);
  }

  /** O rótulo do ponteiro nomeado, com a seta que o liga ao nó. */
  private ponteiro(m: Modelo, rotulo: string, ptr: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const alvo = alvoDe(m, ptr);
    const pose = alvo !== 0 ? this.poses.get(alvo) : undefined;

    const y = pose ? pose.y : MARGEM_TOPO;
    const xNo = pose ? pose.x : this.larguraCss / 2;
    const larguraCaixa = 58;
    const xCaixa = xNo - LARGURA_NO / 2 - 92;

    ctx.globalAlpha = 1;
    this.retangulo(xCaixa, y - 14, larguraCaixa, 28, 6);
    ctx.fillStyle = p.bg3;
    ctx.fill();
    ctx.strokeStyle = p.acentoFraco;
    ctx.lineWidth = 1;
    ctx.stroke();

    ctx.fillStyle = p.acento;
    ctx.font = `600 13px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(rotulo, xCaixa + larguraCaixa / 2, y);

    ctx.strokeStyle = p.acento;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(xCaixa + larguraCaixa, y);

    if (pose) {
      ctx.lineTo(xNo - LARGURA_NO / 2, y);
      ctx.stroke();
      this.setaDireita(xNo - LARGURA_NO / 2, y, p.acento);
    } else {
      ctx.lineTo(xCaixa + larguraCaixa + 22, y);
      ctx.stroke();
      ctx.fillStyle = p.fg3;
      ctx.font = `500 12px ${p.mono}`;
      ctx.textAlign = "left";
      ctx.fillText("NULL", xCaixa + larguraCaixa + 30, y);
    }
  }

  private retangulo(x: number, y: number, w: number, h: number, r: number) {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.arcTo(x + w, y, x + w, y + h, r);
    ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r);
    ctx.arcTo(x, y, x + w, y, r);
    ctx.closePath();
  }

  private setaBaixo(x: number, y: number, cor: string) {
    const ctx = this.ctx;
    ctx.fillStyle = cor;
    ctx.beginPath();
    ctx.moveTo(x, y + 2);
    ctx.lineTo(x - 5, y - 6);
    ctx.lineTo(x + 5, y - 6);
    ctx.closePath();
    ctx.fill();
  }

  private setaDireita(x: number, y: number, cor: string) {
    const ctx = this.ctx;
    ctx.fillStyle = cor;
    ctx.beginPath();
    ctx.moveTo(x + 2, y);
    ctx.lineTo(x - 6, y - 5);
    ctx.lineTo(x - 6, y + 5);
    ctx.closePath();
    ctx.fill();
  }
}
