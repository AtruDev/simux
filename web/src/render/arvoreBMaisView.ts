/* web/src/render/arvoreBMaisView.ts — a árvore B+ desenhada.
 *
 * Herda a página inteira do ArvoreBView: um nó de B+ é uma página com várias
 * chaves, exatamente como o de uma árvore B, e reimplementar o desenho daria
 * duas cópias que divergem na primeira correção.
 *
 * O que este arquivo acrescenta é a única coisa que a B+ tem a mais, e é
 * justamente a coisa que ela existe para ter: a CORRENTE DE FOLHAS. Ela é
 * desenhada com traço tracejado e na horizontal, para não se confundir com as
 * arestas de árvore — é um ponteiro entre páginas irmãs, não uma descida —, e
 * é ela que faz a varredura sequencial parecer o que é: uma linha reta, sem
 * subir uma vez sequer.
 *
 * O elo vem no slot 0 de quem foi anunciado como folha (CAMPO_FOLHA). Numa
 * folha não existe filho, e o ponteiro que sobra na página é o da folha
 * seguinte: é assim na estrutura de verdade, e é assim aqui. */

import { Ptr } from "../core/ops";
import { t } from "../i18n";
import { alvoDe, type Modelo } from "../model/modelo";
import { ArvoreBView } from "./arvoreBView";

export class ArvoreBMaisView extends ArvoreBView {
  protected override desenharNo(m: Modelo, id: number): void {
    /* O elo primeiro, para a página cobrir a ponta da linha — a mesma ordem
     * que a classe base usa entre arestas e nós. */
    this.desenharElo(m, id);
    super.desenharNo(m, id);
    this.rotularInicio(m, id);
  }

  /** A seta horizontal e tracejada até a folha seguinte. */
  private desenharElo(m: Modelo, id: number): void {
    const no = m.nos.get(id);
    if (!no?.folha) return;

    const destino = no.arestas.get(0) ?? 0;
    if (destino === 0) return;

    const de = this.poses.get(id);
    const para = this.poses.get(destino);
    if (!de || !para) return;

    const ctx = this.ctx;
    const p = this.paleta;
    const larguraDe = this.larguraDaPagina(m, id);
    const larguraPara = this.larguraDaPagina(m, destino);
    const x0 = de.x + larguraDe / 2;
    const x1 = para.x - larguraPara / 2;

    /* Folhas fora de ordem na tela não acontecem — a corrente é crescente e o
     * layout também —, mas um trace defeituoso poderia produzir isso, e uma
     * seta para trás atravessando a árvore seria pior que nenhuma. */
    if (x1 <= x0) return;

    ctx.globalAlpha = Math.min(de.alfa, para.alfa) * 0.9;
    ctx.strokeStyle = p.linha2;
    ctx.lineWidth = 1.25;
    ctx.setLineDash([4, 3]);
    ctx.beginPath();
    ctx.moveTo(x0, de.y);
    ctx.lineTo(x1, para.y);
    ctx.stroke();
    ctx.setLineDash([]);

    /* A ponta da seta, à direita: a corrente tem sentido, e é ele que faz a
     * varredura ser uma leitura e não uma vizinhança. */
    const ponta = 5;
    ctx.fillStyle = p.linha2;
    ctx.beginPath();
    ctx.moveTo(x1, para.y);
    ctx.lineTo(x1 - ponta, para.y - ponta * 0.6);
    ctx.lineTo(x1 - ponta, para.y + ponta * 0.6);
    ctx.closePath();
    ctx.fill();

    ctx.globalAlpha = 1;
  }

  /** O rótulo `início`, na ponta da corrente.
   *
   * É o ponteiro que a árvore B não tem, e é dele que a varredura parte — sem
   * passar pela raiz uma vez sequer.
   *
   * Fica EMBAIXO da folha, e não ao lado: a primeira folha é, por definição, a
   * mais à esquerda da tela, e um rótulo à esquerda dela sai pela borda assim
   * que a palavra é um pouco mais longa — o inglês `head` cabia e o português
   * `início` não. Por cima é onde entra a aresta do pai; por baixo não há
   * nada, e o desenho fica igual nos dois idiomas. */
  private rotularInicio(m: Modelo, id: number): void {
    if (alvoDe(m, Ptr.PTR_INICIO) !== id) return;

    const pose = this.poses.get(id);
    if (!pose) return;

    const ctx = this.ctx;
    const p = this.paleta;
    const meia = this.altura() / 2;

    ctx.globalAlpha = pose.alfa;
    ctx.fillStyle = p.acento;
    ctx.font = `600 11px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.fillText(t("rotulo.inicio"), pose.x, pose.y + meia + 12);

    ctx.strokeStyle = p.acento;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(pose.x, pose.y + meia + 10);
    ctx.lineTo(pose.x, pose.y + meia + 4);
    ctx.stroke();
    this.setaCima(pose.x, pose.y + meia + 1, p.acento);
    ctx.globalAlpha = 1;
  }

  /* A mesma conta que o desenho da página faz, e não uma segunda: se as duas
   * divergirem, a seta nasce dentro da caixa. */
  private larguraDaPagina(m: Modelo, id: number): number {
    const no = m.nos.get(id);
    const chaves = Math.max(1, no?.chaves.length ?? 1);

    return this.celula() * chaves;
  }
}
