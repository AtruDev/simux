/* web/src/render/vetorView.ts — células, índices e ponteiros nomeados.
 *
 * Nasce aqui, na pilha com vetor, e não na aba de ordenação: quando a
 * ordenação chegar, este renderizador já existe e já foi depurado numa
 * estrutura de quarenta linhas.
 *
 * O desenho tem duas leituras sobrepostas, e é isso que resolve a fila
 * circular: o índice FÍSICO no canto de cada célula, e a ordem LÓGICA acima
 * dela. Com frente = 5 e fim = 2 num vetor de 8, a fila parece invertida — e
 * só as duas leituras juntas explicam o que se está vendo.
 *
 * A Fase 4 acrescentou o que a busca precisa, e o plano tinha razão em prever
 * o reúso: a faixa viva, a comparação e a chave em mãos. São os mesmos eventos
 * da ordenação, e aqui eles ganham a leitura que faltava — na busca binária
 * importa QUAL valor está em QUAL índice, que é justamente o que as células
 * mostram e as barras não. */

import { Cnt, Ptr, Tag, Tipo } from "../core/ops";
import { alvoDe, contador, ordemLogica, type Modelo } from "../model/modelo";
import { lerPaleta, type Paleta } from "./tokens";

const ALTURA_CELULA = 54;
const LARGURA_MAX = 74;
const LARGURA_MIN = 34;
const VAO = 6;
const RAIO = 6;
const SUAVIZACAO = 0.2;

/** Rótulos que cada estrutura desenha, e de onde eles vêm. */
interface Rotulo {
  ptr: number;
  texto: string;
}

export class VetorView {
  private ctx: CanvasRenderingContext2D;
  private paleta: Paleta;
  private observador: ResizeObserver;
  private larguraCss = 0;
  private alturaCss = 0;
  private semMovimento: boolean;

  /* Brilho por célula, que decai sozinho. Sem isso, uma escrita seria um
   * quadro só e ninguém veria. */
  private brilho = new Map<number, number>();

  constructor(
    private canvas: HTMLCanvasElement,
    private tipo: number,
  ) {
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

  private redimensionar(): void {
    const dpr = window.devicePixelRatio || 1;
    const r = this.canvas.getBoundingClientRect();
    this.larguraCss = r.width;
    this.alturaCss = r.height;
    this.canvas.width = Math.round(r.width * dpr);
    this.canvas.height = Math.round(r.height * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  private ehFila(): boolean {
    return this.tipo === Tipo.TIPO_FILA_VET;
  }

  private ehBusca(): boolean {
    return (
      this.tipo === Tipo.TIPO_BUSCA_SEQ || this.tipo === Tipo.TIPO_BUSCA_BIN
    );
  }

  /* Os rótulos são os identificadores do laço em C, e não texto de interface:
   * `lo`, `hi` e `meio` são o que o painel de código está exibindo ao lado, e
   * traduzi-los faria a tela falar de variáveis que não existem no arquivo. É
   * a mesma decisão de `topo` e `frente`. */
  private rotulos(): Rotulo[] {
    /* A sequencial não tem lo nem hi — ela tem um índice só. Desenhá-los
     * apontando para a célula zero, que é onde um ponteiro nunca emitido cai,
     * faria a tela afirmar que o algoritmo mantém uma faixa que ele não
     * mantém. É metade da comparação sendo contada errado. */
    if (this.tipo === Tipo.TIPO_BUSCA_SEQ) {
      return [{ ptr: Ptr.PTR_CURSOR, texto: "i" }];
    }
    if (this.tipo === Tipo.TIPO_BUSCA_BIN) {
      return [
        { ptr: Ptr.PTR_CURSOR, texto: "meio" },
        { ptr: Ptr.PTR_I, texto: "lo" },
        { ptr: Ptr.PTR_J, texto: "hi" },
      ];
    }
    return this.ehFila()
      ? [
          { ptr: Ptr.PTR_FRENTE, texto: "frente" },
          { ptr: Ptr.PTR_FIM, texto: "fim" },
        ]
      : [{ ptr: Ptr.PTR_TOPO, texto: "topo" }];
  }

  desenhar(m: Modelo): void {
    const k = this.semMovimento ? 1 : SUAVIZACAO;

    /* decaimento do brilho */
    for (const [i, v] of this.brilho) {
      const novo = v * (1 - k);
      if (novo < 0.02) this.brilho.delete(i);
      else this.brilho.set(i, novo);
    }
    if (m.vetor) {
      if (m.vetor.ultimoEscrito >= 0) this.brilho.set(m.vetor.ultimoEscrito, 1);
      if (m.vetor.ultimoLido >= 0) this.brilho.set(m.vetor.ultimoLido, 1);
    }

    this.pintar(m);
  }

  private pintar(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;

    ctx.clearRect(0, 0, this.larguraCss, this.alturaCss);
    if (!m.vetor) return;

    const cap = m.vetor.capacidade;
    const largura = Math.max(
      LARGURA_MIN,
      Math.min(LARGURA_MAX, (this.larguraCss - 80) / cap - VAO),
    );
    const total = cap * largura + (cap - 1) * VAO;
    const x0 = (this.larguraCss - total) / 2;
    const y = this.alturaCss / 2 - ALTURA_CELULA / 2;

    /* A ordem lógica precisa saber onde a fila começa e quantos ela tem. */
    const quantidade = contador(m, Cnt.CNT_TAMANHO);
    const frente = this.ehFila()
      ? alvoDe(m, Ptr.PTR_FRENTE)
      : alvoDe(m, Ptr.PTR_TOPO);
    const logicos = this.ehFila()
      ? ordemLogica(cap, frente, quantidade)
      : [];
    const posicaoLogica = new Map<number, number>();
    logicos.forEach((fisico, ordem) => posicaoLogica.set(fisico, ordem + 1));

    /* A faixa viva da busca binária. Fora dela a célula apaga — é o desenho
     * do algoritmo jogando metade fora, e é a razão de EV_ARR_RANGE existir
     * no vocabulário desde a Fase 0. */
    const faixa = this.ehBusca() ? m.vetor.faixa : null;
    const naFaixa = (i: number) =>
      faixa === null || (i >= faixa[0] && i <= faixa[1]);

    for (let i = 0; i < cap; i++) {
      const x = x0 + i * (largura + VAO);
      const valor = m.vetor.valores[i] ?? null;
      const marca = m.vetor.marcas[i] ?? Tag.TAG_NENHUMA;
      const liberada = marca === Tag.TAG_LIVRE;
      const dentro = this.ehFila()
        ? posicaoLogica.has(i)
        : i < quantidade;
      const viva = dentro && naFaixa(i);
      const comparando =
        m.vetor.comparando !== null && m.vetor.comparando[0] === i;
      const achada = marca === Tag.TAG_PIVO;

      /* O que saiu da faixa perde presença. Além da cor: é a leitura que
       * sobrevive a não distinguir matiz. */
      ctx.save();
      if (dentro && !viva) ctx.globalAlpha = 0.3;

      this.retangulo(x, y, largura, ALTURA_CELULA, RAIO);
      ctx.fillStyle = achada ? p.stPivot : comparando ? p.stCompare : p.bg2;
      ctx.fill();

      /* Traço sólido para o que a estrutura ocupa, tracejado para a célula
       * que já foi ocupada e não é mais. Estado nunca é só matiz. */
      ctx.save();
      if (dentro) {
        ctx.strokeStyle = p.linha2;
        ctx.lineWidth = 1;
      } else if (liberada) {
        ctx.strokeStyle = p.fg3;
        ctx.lineWidth = 1;
        ctx.setLineDash([3, 3]);
      } else {
        ctx.strokeStyle = p.linha;
        ctx.lineWidth = 1;
      }
      ctx.stroke();
      ctx.restore();

      const aceso = comparando || achada ? 0 : (this.brilho.get(i) ?? 0);
      if (aceso > 0) {
        ctx.save();
        ctx.globalAlpha = aceso * 0.5;
        this.retangulo(x, y, largura, ALTURA_CELULA, RAIO);
        ctx.fillStyle = p.stDone;
        ctx.fill();
        ctx.restore();
      }

      if (valor !== null) {
        ctx.fillStyle = dentro || comparando ? p.fg : p.fg3;
        ctx.font = `600 15px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(String(valor), x + largura / 2, y + ALTURA_CELULA / 2);
      }

      /* Índice físico dentro da célula, no canto.
       *
       * Embaixo ele ficava escondido atrás da seta do ponteiro — e é
       * justamente na célula apontada que se quer ler o índice. */
      ctx.fillStyle = p.fg3;
      ctx.font = `500 9px ${p.mono}`;
      ctx.textAlign = "left";
      ctx.textBaseline = "bottom";
      ctx.fillText(String(i), x + 5, y + ALTURA_CELULA - 4);

      /* ordem lógica, só onde ela existe */
      const ordem = posicaoLogica.get(i);
      if (ordem !== undefined) {
        ctx.fillStyle = p.acento;
        ctx.font = `600 10px ${p.mono}`;
        ctx.textBaseline = "bottom";
        ctx.fillText(`${ordem}º`, x + largura / 2, y - 24);
      }

      ctx.restore();
    }

    this.ponteiros(m, x0, largura, y);
    if (this.ehBusca()) this.chave(m, y);
  }

  /** A chave procurada, na célula auxiliar que o C emite antes de buscar.
   *
   * Toda comparação da busca é contra ela — é o `.c = 1` do EV_ARR_COMPARE.
   * Sem desenhá-la, o destaque da célula comparada não diria contra o quê. */
  private chave(m: Modelo, y: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const valor = m.vetor?.aux?.[0] ?? null;
    if (valor === null) return;

    const largura = 74;
    const altura = 34;
    /* Centrada, e não no canto: o canto de cima à esquerda é do nome da faixa
     * no modo comparar, e as duas coisas se sobrepunham. */
    const x = (this.larguraCss - largura) / 2;
    const topo = y - 74;

    this.retangulo(x, topo, largura, altura, RAIO);
    ctx.fillStyle = p.bg3;
    ctx.fill();

    /* Tracejado, como toda memória temporária desta interface: é a mesma
     * convenção da célula liberada e do auxiliar do merge. */
    ctx.save();
    ctx.strokeStyle = p.stAux;
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.stroke();
    ctx.restore();

    ctx.fillStyle = p.fg;
    ctx.font = `600 15px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(valor), x + largura / 2, topo + altura / 2);

    ctx.fillStyle = p.fg3;
    ctx.font = `500 10px ${p.mono}`;
    ctx.textBaseline = "bottom";
    ctx.fillText("chave", x + largura / 2, topo - 4);
  }

  /** Os rótulos nomeados, apontando para a célula de baixo para cima. */
  private ponteiros(m: Modelo, x0: number, largura: number, y: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const rotulos = this.rotulos();

    rotulos.forEach((rotulo, nivel) => {
      /* Ponteiro que a estrutura nunca emitiu não é desenhado. Sem isto ele
       * cairia na célula zero, que é o alvo padrão, e a tela afirmaria algo
       * que o algoritmo não fez. */
      if (!m.ponteiros.has(rotulo.ptr)) return;

      const alvo = alvoDe(m, rotulo.ptr);
      const baseY = y + ALTURA_CELULA + 24 + nivel * 22;

      ctx.fillStyle = p.acento;
      ctx.font = `600 12px ${p.mono}`;
      ctx.textBaseline = "top";

      /* -1 é "nenhum" no mundo do vetor, porque 0 é um índice válido. */
      if (alvo < 0) {
        ctx.textAlign = "left";
        ctx.fillStyle = p.fg3;
        ctx.fillText(`${rotulo.texto} = —`, x0, baseY);
        return;
      }

      const x = x0 + alvo * (largura + VAO) + largura / 2;
      ctx.textAlign = "center";
      ctx.fillText(rotulo.texto, x, baseY);

      ctx.strokeStyle = p.acento;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(x, baseY - 2);
      ctx.lineTo(x, y + ALTURA_CELULA + 2);
      ctx.stroke();

      ctx.fillStyle = p.acento;
      ctx.beginPath();
      ctx.moveTo(x, y + ALTURA_CELULA);
      ctx.lineTo(x - 4, y + ALTURA_CELULA + 6);
      ctx.lineTo(x + 4, y + ALTURA_CELULA + 6);
      ctx.closePath();
      ctx.fill();
    });
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
}
