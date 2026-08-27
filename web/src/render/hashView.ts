/* web/src/render/hashView.ts — a tabela hash com encadeamento separado.
 *
 * É o primeiro desenho do projeto que junta os dois mundos do vocabulário: o
 * arranjo de baldes vem de EV_ARR_INIT/EV_ARR_WRITE, e as cadeias vêm de
 * EV_NODE_NEW/EV_EDGE_SET. Nenhum evento novo foi preciso, e o motivo é que a
 * estrutura é literalmente isso — um vetor de ponteiros, cada um para uma
 * lista.
 *
 * O truque que faz os dois mundos se encontrarem: o valor guardado na célula
 * do balde é o ID DO NÓ da cabeça da cadeia, e 0 é balde vazio. É o mesmo
 * significado que o `0` tem em toda aresta do projeto, e é o que deixa este
 * arquivo seguir a cadeia a partir de um índice do arranjo.
 *
 * Baldes numa coluna e cadeias para a direita, e não o contrário: é o desenho
 * do livro, e é o que faz a cadeia comprida — o sintoma de um `m` mal
 * escolhido — saltar aos olhos como uma linha que atravessa a tela. */

import { Cnt, Ptr } from "../core/ops";
import { alvoDe, contador, type Modelo } from "../model/modelo";
import { Tela } from "./tela";

const SLOT_PROX = 0;

const MARGEM_X = 18;
const MARGEM_TOPO = 26;
const LARGURA_BALDE = 46;
const LARGURA_NO = 44;
const VAO_NO = 26;
/* O piso é baixo de propósito: no modo comparar são quatro tabelas em quatro
 * faixas, e uma tabela de m = 11 tem que caber em cento e poucos pixels. A
 * fonte encolhe junto; abaixo disso o número do balde deixa de ser legível, e
 * aí é melhor a coluna transbordar do que virar uma pilha de riscos. */
const ALTURA_MIN = 11;
const ALTURA_MAX = 34;

export class HashView extends Tela {
  private alturaLinha = ALTURA_MAX;

  desenhar(m: Modelo): void {
    const baldes = m.vetor?.capacidade ?? 0;

    if (baldes > 0) {
      const util = this.alturaCss - MARGEM_TOPO - 10;
      this.alturaLinha = Math.max(
        ALTURA_MIN,
        Math.min(ALTURA_MAX, util / baldes),
      );
    }

    /* Cada nó é posicionado pela sua POSIÇÃO NA CADEIA, e não pela ordem de
     * criação: inserir na cabeça empurra todo mundo uma casa para a direita, e
     * é o tween que faz esse empurrão parecer um empurrão. */
    const alcancados = new Set<number>();

    for (let b = 0; b < baldes; b++) {
      const y = this.linhaY(b);
      let id = m.vetor?.valores[b] ?? 0;
      let passo = 0;

      while (id !== 0 && m.nos.has(id) && !alcancados.has(id)) {
        alcancados.add(id);

        const x =
          MARGEM_X + LARGURA_BALDE + VAO_NO + passo * (LARGURA_NO + VAO_NO);

        this.mirar(id, x, y, {
          x: x - 20,
          y,
          alvoX: x,
          alvoY: y,
          alfa: 0,
          alvoAlfa: 1,
        });

        id = m.nos.get(id)?.arestas.get(SLOT_PROX) ?? 0;
        passo++;
      }
    }

    /* O nó existe antes de a célula do balde apontar para ele — é o passo
     * entre EV_NODE_NEW e EV_ARR_WRITE. Escondê-lo tornaria um bug de trace
     * invisível. */
    const orfaos = m.ordem.filter((id) => !alcancados.has(id));
    orfaos.forEach((id, k) => {
      const x = this.larguraCss - MARGEM_X - LARGURA_NO / 2;
      const y = this.alturaCss - 16 - k * 8;

      this.mirar(id, x, y, {
        x,
        y: y + 12,
        alvoX: x,
        alvoY: y,
        alfa: 0,
        alvoAlfa: 1,
      });
    });

    this.animar((id) => m.nos.has(id));
    this.pintar(m, baldes);
  }

  private linhaY(balde: number): number {
    return MARGEM_TOPO + balde * this.alturaLinha + this.alturaLinha / 2;
  }

  private pintar(m: Modelo, baldes: number): void {
    const ctx = this.ctx;
    const p = this.paleta;

    this.limpar();
    if (baldes === 0) return;

    const escolhido = m.ponteiros.has(Ptr.PTR_BALDE)
      ? alvoDe(m, Ptr.PTR_BALDE)
      : -1;
    const altura = Math.max(12, this.alturaLinha - 4);
    const corpo = Math.max(8, Math.round(altura * 0.46));

    for (let b = 0; b < baldes; b++) {
      const y = this.linhaY(b);
      const topo = y - altura / 2;
      const cabeca = m.vetor?.valores[b] ?? 0;
      const eleito = b === escolhido;

      this.retangulo(MARGEM_X, topo, LARGURA_BALDE, altura, 4);
      ctx.fillStyle = p.bg2;
      ctx.fill();

      /* O acento marca o balde que h(k) escolheu — é onde a interface está
       * olhando, que é o único uso dele que a regra de cor permite. */
      ctx.strokeStyle = eleito ? p.acento : p.linha;
      ctx.lineWidth = eleito ? 2 : 1;
      ctx.stroke();

      ctx.fillStyle = eleito ? p.acentoAlto : p.fg3;
      ctx.font = `500 ${Math.max(8, corpo - 1)}px ${p.mono}`;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(String(b), MARGEM_X + LARGURA_BALDE / 2, y);

      /* O traço que liga o balde à cabeça da cadeia. Balde vazio não ganha
       * traço nenhum: o vazio tem que ser visível como vazio. */
      const pose = cabeca !== 0 ? this.poses.get(cabeca) : undefined;
      if (pose) {
        ctx.globalAlpha = pose.alfa;
        ctx.strokeStyle = p.linha2;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(MARGEM_X + LARGURA_BALDE, y);
        ctx.lineTo(pose.x - LARGURA_NO / 2, y);
        ctx.stroke();
        this.setaDireita(pose.x - LARGURA_NO / 2, y, p.linha2);
        ctx.globalAlpha = 1;
      }
    }

    /* As arestas da cadeia, antes dos nós. */
    for (const id of m.ordem) {
      const pose = this.poses.get(id);
      const destino = m.nos.get(id)?.arestas.get(SLOT_PROX) ?? 0;
      const poseDestino = destino !== 0 ? this.poses.get(destino) : undefined;
      if (!pose || !poseDestino) continue;

      ctx.globalAlpha = Math.min(pose.alfa, poseDestino.alfa);
      ctx.strokeStyle = p.linha2;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(pose.x + LARGURA_NO / 2, pose.y);
      ctx.lineTo(poseDestino.x - LARGURA_NO / 2, poseDestino.y);
      ctx.stroke();
      this.setaDireita(poseDestino.x - LARGURA_NO / 2, poseDestino.y, p.linha2);
      ctx.globalAlpha = 1;
    }

    for (const id of m.ordem) {
      const pose = this.poses.get(id);
      const no = m.nos.get(id);
      if (!pose || !no) continue;

      ctx.globalAlpha = pose.alfa;

      this.retangulo(
        pose.x - LARGURA_NO / 2,
        pose.y - altura / 2,
        LARGURA_NO,
        altura,
        4,
      );
      ctx.fillStyle = p.bg3;
      ctx.fill();

      const visitado = m.visitados.has(id);
      ctx.strokeStyle = visitado ? p.acento : p.linha2;
      ctx.lineWidth = visitado ? 2 : 1;
      ctx.stroke();

      ctx.fillStyle = p.fg;
      ctx.font = `600 ${corpo}px ${p.mono}`;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(String(no.valor), pose.x, pose.y);

      ctx.globalAlpha = 1;
    }

    this.rodape(m, baldes);
  }

  /** O fator de carga, que é o número que resume a tabela inteira.
   *
   * À direita, e não à esquerda: o canto de cima à esquerda é do nome da faixa
   * no modo comparar, e as duas coisas se sobrepunham. */
  private rodape(m: Modelo, baldes: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const n = contador(m, Cnt.CNT_TAMANHO);

    ctx.fillStyle = p.fg3;
    ctx.font = `500 10px ${p.mono}`;
    ctx.textAlign = "right";
    ctx.textBaseline = "top";
    ctx.fillText(
      `n/m = ${n}/${baldes} = ${(n / baldes).toFixed(2)}`,
      this.larguraCss - MARGEM_X,
      6,
    );
  }
}
