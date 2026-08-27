/* web/src/render/ordenacaoView.ts — o vetor da ordenação, em barras.
 *
 * O VetorView da Fase 2 desenha células com o número dentro, e é o desenho
 * certo para uma pilha de oito posições: ali o que importa é QUAL valor está
 * em QUAL índice. Aqui o que importa é a forma — ordenar é a silhueta virando
 * uma rampa —, e vinte células com número não viram forma nenhuma. Os dois
 * lêem o mesmo VetorModelo e o mesmo aplicar.ts, que é onde o reúso vale.
 *
 * As três regras de cor do projeto valem aqui inteiras:
 *
 *   1. O acento é da interface. Nenhuma barra é pintada com ele — só os
 *      cursores i, j e mín, que são exatamente "onde a interface está
 *      olhando".
 *   2. Identidade de algoritmo é linha fina, e por isso não aparece neste
 *      arquivo: o quicksort é uma régua no cabeçalho, nunca a cor de uma
 *      barra. O que preenche barra é ESTADO.
 *   3. Nenhum estado é distinguido só por matiz. Comparando e pivô ganham
 *      traço sólido por cima do preenchimento; o auxiliar é tracejado, porque
 *      é memória temporária; o que está fora da faixa ativa perde saturação e
 *      contorno. A legenda fica sempre visível, no painel ao lado. */

import { Ptr, Tag } from "../core/ops";
import { alvoDe, type Modelo } from "../model/modelo";
import { lerPaleta, type Paleta } from "./tokens";

const MARGEM_X = 16;
const MARGEM_TOPO = 14;
/* Espaço embaixo para os índices e para os cursores i/j/mín. */
const FAIXA_RODAPE = 30;
/* Altura da tira do auxiliar, quando ela existe. */
const ALTURA_AUX = 44;
const VAO_MAX = 4;
const SUAVIZACAO = 0.2;

/* Abaixo desta largura de barra o número não cabe, e escrever mesmo assim
 * vira um borrão cinza atravessando o gráfico. */
const LARGURA_PARA_NUMERO = 22;
const LARGURA_PARA_INDICE = 26;

export class OrdenacaoView {
  private ctx: CanvasRenderingContext2D;
  private paleta: Paleta;
  private observador: ResizeObserver;
  private larguraCss = 0;
  private alturaCss = 0;
  private semMovimento: boolean;

  /* Brilho por célula, que decai sozinho.
   *
   * Uma comparação dura um quadro. Em 16× de velocidade ela dura menos que
   * isso — o Player aplica vários eventos entre dois desenhos. O brilho que
   * decai é o que faz o rastro do algoritmo ficar visível mesmo quando a
   * reprodução passa mais rápido que a tela. */
  private brilho = new Map<number, number>();

  /* O que o modelo dizia no quadro anterior.
   *
   * O modelo guarda o ÚLTIMO índice escrito, e ele continua sendo o último
   * depois de a animação parar — verdade que não interessa mais. Comparando
   * com o quadro anterior, o brilho acende na MUDANÇA e decai sozinho: o
   * vermelho da escrita vira um pulso, e o vetor em repouso volta a ser
   * neutro. Sem isto, a última célula gerada ficava acesa para sempre. */
  private escritoAntes = -1;
  private lidoAntes = -1;

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

  private redimensionar(): void {
    const dpr = window.devicePixelRatio || 1;
    const r = this.canvas.getBoundingClientRect();
    this.larguraCss = r.width;
    this.alturaCss = r.height;
    this.canvas.width = Math.round(r.width * dpr);
    this.canvas.height = Math.round(r.height * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  desenhar(m: Modelo): void {
    const k = this.semMovimento ? 1 : SUAVIZACAO;

    for (const [i, v] of this.brilho) {
      const novo = v * (1 - k);
      if (novo < 0.02) this.brilho.delete(i);
      else this.brilho.set(i, novo);
    }

    const vetor = m.vetor;
    if (vetor) {
      if (vetor.ultimoEscrito >= 0 && vetor.ultimoEscrito !== this.escritoAntes) {
        this.brilho.set(vetor.ultimoEscrito, 1);
      }
      if (vetor.ultimoLido >= 0 && vetor.ultimoLido !== this.lidoAntes) {
        this.brilho.set(vetor.ultimoLido, 1);
      }
      this.escritoAntes = vetor.ultimoEscrito;
      this.lidoAntes = vetor.ultimoLido;

      if (vetor.comparando) {
        this.brilho.set(vetor.comparando[0], 1);
        if (!vetor.comparandoMao) this.brilho.set(vetor.comparando[1], 1);
      }
    }

    this.pintar(m);
  }

  private pintar(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;

    ctx.clearRect(0, 0, this.larguraCss, this.alturaCss);

    const vetor = m.vetor;
    if (!vetor || vetor.capacidade === 0) return;

    const n = vetor.capacidade;
    const temAux = vetor.aux !== null && vetor.aux.length > 0;

    const alturaAux = temAux ? ALTURA_AUX : 0;
    const base = this.alturaCss - FAIXA_RODAPE - alturaAux;
    const alturaUtil = base - MARGEM_TOPO;
    if (alturaUtil <= 0) return;

    const passo = (this.larguraCss - 2 * MARGEM_X) / n;
    const vao = Math.min(VAO_MAX, passo * 0.18);
    const largura = Math.max(1, passo - vao);

    /* A escala é o maior valor JÁ VISTO na capacidade, e não o maior do
     * instante: se ela seguisse o instante, trocar duas barras mudaria a
     * altura de todas as outras, e a animação inteira tremeria. */
    let maximo = 1;
    for (const valor of vetor.valores) {
      if (valor !== null && valor > maximo) maximo = valor;
    }

    const faixa = vetor.faixa;
    const dentroDaFaixa = (i: number) =>
      faixa === null || (i >= faixa[0] && i <= faixa[1]);

    for (let i = 0; i < n; i++) {
      const valor = vetor.valores[i] ?? null;
      if (valor === null) continue;

      const x = MARGEM_X + i * passo;
      const altura = Math.max(2, (valor / maximo) * alturaUtil);
      const y = base - altura;
      const ativa = dentroDaFaixa(i);

      this.barra(m, i, x, y, largura, altura, ativa);

      if (largura >= LARGURA_PARA_NUMERO) {
        ctx.fillStyle = ativa ? p.fg : p.fg3;
        ctx.font = `600 11px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "bottom";
        ctx.fillText(String(valor), x + largura / 2, y - 3);
      }
      if (largura >= LARGURA_PARA_INDICE) {
        ctx.fillStyle = p.fg3;
        ctx.font = `500 9px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "top";
        ctx.fillText(String(i), x + largura / 2, base + 4);
      }
    }

    this.contornoDaFaixa(faixa, n, passo, base, alturaUtil);
    this.cursores(m, passo, largura, base);
    if (temAux) this.auxiliar(m, base, maximo, passo);
  }

  /** Uma barra: preenchimento pelo estado, traço pelo mesmo estado. */
  private barra(
    m: Modelo,
    i: number,
    x: number,
    y: number,
    largura: number,
    altura: number,
    ativa: boolean,
  ): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const vetor = m.vetor;
    if (!vetor) return;

    const marca = vetor.marcas[i] ?? Tag.TAG_NENHUMA;
    const comparando =
      vetor.comparando !== null &&
      (vetor.comparando[0] === i ||
        (!vetor.comparandoMao && vetor.comparando[1] === i));

    /* A escrita é um pulso, não um estado. Ela vale enquanto o brilho daquela
     * célula ainda está alto — depois disso a barra volta a mostrar o que ela
     * é, que é o que a marca diz. */
    const aceso = this.brilho.get(i) ?? 0;
    const escrita = vetor.ultimoEscrito === i && aceso > 0.35;

    /* A ordem é de precedência, e não estética: o que o algoritmo está
     * tocando AGORA ganha da marca que ficou de antes. */
    let cor = p.bg3;
    let traco: string | null = null;
    let tracejado = false;

    if (marca === Tag.TAG_ORDENADO) cor = p.stDone;
    if (marca === Tag.TAG_PIVO) {
      cor = p.stPivot;
      traco = p.fg;
    }
    if (marca === Tag.TAG_LIVRE) {
      cor = p.bg2;
      traco = p.fg3;
      tracejado = true;
    }
    if (escrita) {
      cor = p.stSwap;
      traco = p.fg;
    }
    if (comparando) {
      cor = p.stCompare;
      traco = p.fg;
    }

    ctx.save();
    /* Fora da faixa ativa a barra perde presença. É a terceira leitura, além
     * do preenchimento e do traço: dá para ver onde o algoritmo está mesmo
     * sem distinguir as cores. */
    ctx.globalAlpha = ativa ? 1 : 0.32;

    ctx.fillStyle = cor;
    ctx.fillRect(x, y, largura, altura);

    if (aceso > 0.02 && !comparando && !escrita) {
      ctx.globalAlpha = ativa ? aceso * 0.45 : aceso * 0.2;
      ctx.fillStyle = p.stCompare;
      ctx.fillRect(x, y, largura, altura);
      ctx.globalAlpha = ativa ? 1 : 0.32;
    }

    if (traco !== null && largura >= 3) {
      ctx.strokeStyle = traco;
      ctx.lineWidth = 1;
      if (tracejado) ctx.setLineDash([3, 3]);
      ctx.strokeRect(x + 0.5, y + 0.5, largura - 1, altura - 1);
    }
    ctx.restore();
  }

  /** O subvetor ativo, desenhado como um colchete por baixo do gráfico. */
  private contornoDaFaixa(
    faixa: readonly [number, number] | null,
    n: number,
    passo: number,
    base: number,
    alturaUtil: number,
  ): void {
    if (faixa === null) return;
    /* A faixa que cobre tudo não é informação: desenhá-la só acrescentaria
     * uma linha que nunca sai do lugar. */
    if (faixa[0] <= 0 && faixa[1] >= n - 1) return;

    const ctx = this.ctx;
    const p = this.paleta;
    const x0 = MARGEM_X + faixa[0] * passo;
    const x1 = MARGEM_X + (faixa[1] + 1) * passo;
    const topo = base - alturaUtil - 4;

    ctx.save();
    ctx.strokeStyle = p.linha2;
    ctx.lineWidth = 1;
    ctx.setLineDash([2, 4]);
    ctx.beginPath();
    ctx.moveTo(x0, topo);
    ctx.lineTo(x0, base + 1);
    ctx.moveTo(x1, topo);
    ctx.lineTo(x1, base + 1);
    ctx.stroke();
    ctx.restore();
  }

  /** i, j e mín — os índices nomeados que o algoritmo move.
   *
   * São os únicos desenhos em cor de acento nesta tela, e é justamente o caso
   * que a regra abre: eles não são estado do dado, são onde a interface está
   * olhando. */
  private cursores(
    m: Modelo,
    passo: number,
    largura: number,
    base: number,
  ): void {
    const ctx = this.ctx;
    const p = this.paleta;

    const marcadores: Array<[number, string]> = [
      [Ptr.PTR_I, "i"],
      [Ptr.PTR_J, "j"],
      [Ptr.PTR_MIN, "mín"],
    ];

    let nivel = 0;
    for (const [ptr, texto] of marcadores) {
      if (!m.ponteiros.has(ptr)) continue;
      const alvo = alvoDe(m, ptr);
      if (alvo < 0) continue;

      const x = MARGEM_X + alvo * passo + largura / 2;
      const y = base + 15 + nivel * 0;

      ctx.fillStyle = p.acento;
      ctx.beginPath();
      ctx.moveTo(x, base + 2);
      ctx.lineTo(x - 4, base + 9);
      ctx.lineTo(x + 4, base + 9);
      ctx.closePath();
      ctx.fill();

      /* O rótulo só cabe quando as barras são largas. Com n = 200 a seta
       * sozinha já diz onde é, e três letras empilhadas viram sujeira. */
      if (passo >= 18) {
        ctx.fillStyle = p.acentoAlto;
        ctx.font = `600 10px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "top";
        ctx.fillText(texto, x, y);
      }
      nivel++;
    }
  }

  /** O buffer auxiliar, numa tira embaixo e sempre tracejado.
   *
   * Uma célula na inserção e no shell — o valor em mãos; n células no merge.
   * O tracejado não é enfeite: é o que diz "esta memória é temporária", e é o
   * mesmo tratamento que a célula liberada da pilha com vetor recebe. */
  private auxiliar(
    m: Modelo,
    base: number,
    maximo: number,
    passoPrincipal: number,
  ): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const aux = m.vetor?.aux;
    if (!aux) return;

    const n = aux.length;
    const topo = base + FAIXA_RODAPE;

    /* Uma célula só — o valor em mãos da inserção e do shell — não vira uma
     * barra da largura da tela. O auxiliar usa o mesmo passo do vetor de cima,
     * e as duas tiras ficam alinhadas: no merge, cada célula do auxiliar cai
     * debaixo da célula do vetor que lhe corresponde. */
    const passo = Math.min((this.larguraCss - 2 * MARGEM_X) / n, passoPrincipal);
    const vao = Math.min(VAO_MAX, passo * 0.18);
    const largura = Math.max(1, passo - vao);
    const alturaUtil = ALTURA_AUX - 12;

    ctx.save();
    ctx.strokeStyle = p.linha;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(MARGEM_X, topo - 6);
    ctx.lineTo(this.larguraCss - MARGEM_X, topo - 6);
    ctx.stroke();
    ctx.restore();

    for (let i = 0; i < n; i++) {
      const valor = aux[i] ?? null;
      if (valor === null) continue;

      const x = MARGEM_X + i * passo;
      const altura = Math.max(2, (valor / maximo) * alturaUtil);
      const y = topo + alturaUtil - altura;

      ctx.save();
      ctx.globalAlpha = m.vetor?.auxUltimoEscrito === i ? 1 : 0.75;
      ctx.fillStyle = p.stAux;
      ctx.fillRect(x, y, largura, altura);
      ctx.strokeStyle = p.fg3;
      ctx.lineWidth = 1;
      ctx.setLineDash([3, 3]);
      if (largura >= 3) {
        ctx.strokeRect(x + 0.5, y + 0.5, largura - 1, altura - 1);
      }
      ctx.restore();

      if (largura >= LARGURA_PARA_NUMERO) {
        ctx.fillStyle = p.fg2;
        ctx.font = `600 10px ${p.mono}`;
        ctx.textAlign = "center";
        ctx.textBaseline = "bottom";
        ctx.fillText(String(valor), x + largura / 2, y - 2);
      }
    }
  }
}
