/* web/src/render/arvoreBView.ts — a árvore B desenhada.
 *
 * Herda o layout do ArvoreView inteiro. Reingold–Tilford não mudou uma linha
 * para caber aqui: o que a árvore B trouxe de diferente — N filhos em vez de
 * dois, e nós de largura variável — entrou pelos dois ganchos que o layout já
 * consultava. Duplicar o cálculo de contorno teria dado duas cópias, e a
 * segunda ficaria para trás na primeira correção.
 *
 * O que este arquivo faz de próprio é DESENHAR o nó, e aí a diferença é
 * grande: um nó de árvore B é uma página com várias chaves em células, e não
 * um círculo com um número. É essa forma que explica a estrutura — muitas
 * chaves por página, poucas páginas por busca. */

import type { Modelo } from "../model/modelo";
import { ArvoreView } from "./arvoreView";

/* Largura de uma célula de chave, em unidades de layout. O layout conta uma
 * unidade por chave; aqui essa unidade vira pixels. */
const LARGURA_CHAVE = 34;
const ALTURA_MIN = 20;
const ALTURA_MAX = 30;

export class ArvoreBView extends ArvoreView {
  /* Uma unidade por chave: um nó de cinco chaves é cinco vezes mais largo que
   * um de uma, e o layout afasta os irmãos o suficiente para eles não se
   * encostarem. */
  protected override largura(m: Modelo, id: number): number {
    const no = m.nos.get(id);
    return Math.max(1, no?.chaves.length ?? 1);
  }

  protected override desenharNo(m: Modelo, id: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const no = m.nos.get(id);
    const pose = this.poses.get(id);
    if (!no || !pose) return;

    const chaves = no.chaves.length > 0 ? no.chaves : [no.valor];
    const celula = this.celula();
    const x = pose.x;
    const y = pose.y;
    const altura = this.altura();
    const largura = celula * chaves.length;
    const esquerda = x - largura / 2;
    const topo = y - altura / 2;
    const corpo = Math.max(8, Math.round(celula * 0.4));

    const visitado = m.visitados.has(id);

    ctx.globalAlpha = pose.alfa;

    /* A página inteira é uma caixa só: é ela que é lida do disco de uma vez, e
     * o desenho tem que dizer isso antes de dizer qualquer outra coisa. */
    this.retangulo(esquerda, topo, largura, altura, 4);
    ctx.fillStyle = p.bg2;
    ctx.fill();
    ctx.strokeStyle = visitado ? p.acento : p.linha2;
    ctx.lineWidth = visitado ? 2 : 1;
    ctx.stroke();

    /* As divisórias entre as chaves, finas: elas separam sem competir com a
     * borda da página. */
    ctx.strokeStyle = p.linha;
    ctx.lineWidth = 1;
    for (let i = 1; i < chaves.length; i++) {
      const cx = esquerda + i * celula;
      ctx.beginPath();
      ctx.moveTo(cx, topo + 3);
      ctx.lineTo(cx, topo + altura - 3);
      ctx.stroke();
    }

    ctx.fillStyle = p.fg;
    ctx.font = `600 ${corpo}px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    chaves.forEach((chave, i) => {
      ctx.fillText(String(chave), esquerda + (i + 0.5) * celula, y);
    });

    /* O número da página, discreto e à esquerda. É o que liga o desenho aos
     * eventos de disco do log: quando o painel diz "leu a página 7", dá para
     * apontar qual caixa é. */
    if (no.pagina !== null && celula >= 20) {
      ctx.fillStyle = p.fg3;
      ctx.font = `500 ${Math.max(8, corpo - 3)}px ${p.mono}`;
      ctx.textAlign = "right";
      ctx.textBaseline = "bottom";
      ctx.fillText(`p${no.pagina}`, esquerda - 4, topo + altura);
    }

    ctx.globalAlpha = 1;
  }

  /** As arestas saem de baixo da célula certa, e não do centro do nó.
   *
   * O filho `i` fica entre as chaves `i-1` e `i`, e é dessa fronteira que a
   * linha desce. Sair todas do centro esconderia justamente a relação que faz
   * a busca funcionar. */
  protected override saidaDaAresta(
    m: Modelo,
    id: number,
    indice: number,
    x: number,
    y: number,
  ): { x: number; y: number } {
    const no = m.nos.get(id);
    const chaves = Math.max(1, no?.chaves.length ?? 1);
    const celula = this.celula();
    const largura = celula * chaves;

    return {
      x: x - largura / 2 + indice * celula,
      y: y + this.altura() / 2,
    };
  }

  protected override entradaDaAresta(
    m: Modelo,
    id: number,
    x: number,
    y: number,
  ): { x: number; y: number } {
    void m;
    void id;
    return { x, y: y - this.altura() / 2 };
  }

  /* A célula segue o raio que a classe base calculou por quadro: com a árvore
   * larga ou alta, ele encolhe, e as páginas encolhem junto. */
  private celula(): number {
    return Math.max(14, Math.min(LARGURA_CHAVE, this.raio * 1.7));
  }

  private altura(): number {
    return Math.max(ALTURA_MIN, Math.min(ALTURA_MAX, this.celula() * 0.85));
  }
}
