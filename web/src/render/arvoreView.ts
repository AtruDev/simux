/* web/src/render/arvoreView.ts — a árvore desenhada por Reingold–Tilford.
 *
 * O layout ingênuo — `x = pai.x ± largura / 2^nivel` — é proibido no projeto, e
 * a razão aparece na primeira árvore de dez nós: ele reserva espaço para uma
 * árvore CHEIA, então uma árvore magra desenha um deserto no meio e uma árvore
 * de quinze níveis pede uma tela de quilômetros. Reingold–Tilford resolve isso
 * com uma ideia só: cada subárvore é desenhada por si, e duas subárvores
 * irmãs são afastadas apenas o suficiente para não encostarem.
 *
 * "O suficiente" se mede pelos CONTORNOS: o perfil direito da subárvore
 * esquerda e o perfil esquerdo da direita, nível a nível. O afastamento é o
 * maior aperto encontrado. É por isso que a árvore fica compacta sem nunca se
 * sobrepor, e é por isso que o desenho é estável — mexer numa folha não move
 * o outro lado da árvore.
 *
 * Uma coisa aqui NÃO é Reingold–Tilford, e é deliberada. No original, um nó
 * com um filho só fica exatamente acima dele; aqui ele fica meio passo para o
 * lado contrário. Sem isso, uma árvore degenerada — inserir 1, 2, 3, 4… numa
 * ABB — desenharia uma linha vertical, escondendo justamente a degeneração
 * que a aba existe para mostrar. Com o desvio, ela vira a escada que a AVL vai
 * aparecer ao lado para desentortar. */

import { Ptr } from "../core/ops";
import { alvoDe, filhosDe, type Modelo } from "../model/modelo";
import { Tela } from "./tela";

const RAIO_MAX = 19;
/* Abaixo disto o número dentro do nó deixa de ser legível, e é melhor a
 * árvore transbordar do que virar bolinhas mudas. */
const RAIO_MIN = 9;
const MARGEM_TOPO = 54;
const MARGEM_BAIXO = 18;
const MARGEM_X = 28;
/* Distância vertical entre um nível e o seguinte, quando há espaço. */
const NIVEL_MAX = 62;

/* Afastamento mínimo entre dois nós no mesmo nível, em unidades de layout.
 * O layout é calculado nessas unidades e escalado depois, para a árvore caber
 * na tela sem o algoritmo precisar saber o tamanho do canvas. */
const SEPARACAO = 1;

/* Uma subárvore já disposta, em coordenadas relativas à própria raiz. */
interface Disposto {
  id: number;
  filhos: Array<{ no: Disposto; dx: number }>;
  /* Por nível (0 é a raiz desta subárvore), o menor e o maior x. */
  contornoEsq: number[];
  contornoDir: number[];
}

export class ArvoreView extends Tela {
  /* O tamanho do nó e o passo entre níveis são calculados por quadro, a partir
   * do que a árvore virou. Uma ABB degenerada tem altura n e uma AVL tem
   * altura log n; com um passo fixo, a primeira sairia pela borda de baixo — e
   * é justamente ela que precisa ser vista inteira, porque a degeneração é o
   * argumento. */
  protected raio = RAIO_MAX;
  protected passoY = NIVEL_MAX;

  desenhar(m: Modelo): void {
    const raiz = alvoDe(m, Ptr.PTR_RAIZ);
    const disposto = this.dispor(m, raiz, new Set());

    /* Coordenadas em unidades de layout, e só depois escaladas: o algoritmo
     * não sabe o tamanho do canvas, e não precisa saber. */
    const alvos = new Map<number, { x: number; nivel: number }>();
    if (disposto) this.achatar(disposto, 0, 0, alvos);

    const orfaos = this.orfaos(m, alvos);

    let niveis = 1;
    for (const pos of alvos.values()) {
      niveis = Math.max(niveis, pos.nivel + 1);
    }

    const alturaUtil = this.alturaCss - MARGEM_TOPO - MARGEM_BAIXO;
    this.passoY = Math.min(NIVEL_MAX, alturaUtil / Math.max(1, niveis - 1));
    this.raio = Math.max(
      RAIO_MIN,
      Math.min(RAIO_MAX, this.passoY * 0.32),
    );

    const escala = this.escalar(alvos);

    for (const [id, pos] of alvos) {
      const x = this.larguraCss / 2 + pos.x * escala;
      const y = MARGEM_TOPO + pos.nivel * this.passoY;

      /* Nasce onde vai ficar, mas transparente e um pouco acima: o nó novo
       * desce até o lugar em vez de aparecer pronto. */
      this.mirar(id, x, y, {
        x,
        y: y - 22,
        alvoX: x,
        alvoY: y,
        alfa: 0,
        alvoAlfa: 1,
      });
    }

    /* O nó recém-criado existe antes de alguém apontar para ele: é o passo
     * entre EV_NODE_NEW e EV_EDGE_SET. Escondê-lo tornaria um bug de trace
     * invisível; desenhá-lo fora da árvore diz exatamente o que ele é. */
    orfaos.forEach((id, k) => {
      const x = MARGEM_X + this.raio + k * (this.raio * 2 + 16);
      const y = this.alturaCss - this.raio - 6;

      this.mirar(id, x, y, {
        x,
        y: y + 16,
        alvoX: x,
        alvoY: y,
        alfa: 0,
        alvoAlfa: 1,
      });
    });

    this.animar((id) => m.nos.has(id));
    this.pintar(m);
  }

  /* ---- Reingold–Tilford ------------------------------------------------ */

  /** Quantas unidades de layout o nó ocupa.
   *
   * Uma por chave: um nó de árvore binária vale 1, e um nó de árvore B com
   * cinco chaves vale 5. É o único lugar em que a largura entra no algoritmo,
   * e é por isso que Reingold–Tilford serve às duas sem saber qual está
   * desenhando. */
  protected largura(m: Modelo, id: number): number {
    const no = m.nos.get(id);
    return Math.max(1, no?.chaves.length ?? 1);
  }

  /** Dispõe a subárvore de `id`, em coordenadas relativas à própria raiz. */
  private dispor(m: Modelo, id: number, visto: Set<number>): Disposto | null {
    /* `visto` protege contra um ciclo vindo de um trace defeituoso. Uma árvore
     * não tem ciclos; um bug de EV_EDGE_SET, sim, e travar a aba inteira num
     * laço infinito seria o pior jeito de descobrir isso. */
    if (id === 0 || !m.nos.has(id) || visto.has(id)) return null;
    visto.add(id);

    const meia = this.largura(m, id) / 2;
    const dispostos = filhosDe(m, id)
      .map((filho) => this.dispor(m, filho, visto))
      .filter((d): d is Disposto => d !== null);

    const filhos: Disposto["filhos"] = [];

    if (dispostos.length === 1) {
      /* Meio passo para o lado do filho: é o desvio deliberado do algoritmo
       * original, e é o que faz a árvore degenerada parecer degenerada. O
       * lado é o do slot que o filho ocupa — à esquerda, se for o primeiro. */
      const ehPrimeiro = filhosDe(m, id)[0] === dispostos[0]!.id;
      filhos.push({
        no: dispostos[0]!,
        dx: ehPrimeiro ? -SEPARACAO / 2 : +SEPARACAO / 2,
      });
    } else if (dispostos.length > 1) {
      /* Empurra cada irmão para a direita do anterior, pelo APERTO: para cada
       * nível em que os dois existem, quanto o de baixo teria que andar para
       * não encostar no de cima. O maior deles manda. */
      const posicoes: number[] = [0];

      for (let k = 1; k < dispostos.length; k++) {
        const anterior = dispostos[k - 1]!;
        const atual = dispostos[k]!;
        let afastamento = SEPARACAO;
        const comuns = Math.min(
          anterior.contornoDir.length,
          atual.contornoEsq.length,
        );

        for (let n = 0; n < comuns; n++) {
          const aperto =
            anterior.contornoDir[n]! +
            posicoes[k - 1]! -
            atual.contornoEsq[n]! +
            SEPARACAO;
          if (aperto > afastamento) afastamento = aperto;
        }
        posicoes.push(afastamento);
      }

      /* Centra o conjunto sob o pai. */
      const centro = (posicoes[0]! + posicoes[posicoes.length - 1]!) / 2;
      dispostos.forEach((no, k) => {
        filhos.push({ no, dx: posicoes[k]! - centro });
      });
    }

    return { id, filhos, ...this.contornos(filhos, meia) };
  }

  /** Os dois perfis da subárvore, nível a nível, a partir dos filhos.
   *
   * O perfil do nível 0 são as BORDAS do próprio nó, e não o centro dele: com
   * nós de largura variável, é a borda que encosta na do vizinho. */
  private contornos(
    filhos: Disposto["filhos"],
    meia: number,
  ): {
    contornoEsq: number[];
    contornoDir: number[];
  } {
    const contornoEsq = [-meia];
    const contornoDir = [+meia];

    let profundidade = 0;
    for (const { no } of filhos) {
      profundidade = Math.max(profundidade, no.contornoEsq.length);
    }

    for (let k = 0; k < profundidade; k++) {
      let menor = Infinity;
      let maior = -Infinity;

      for (const { no, dx } of filhos) {
        if (k >= no.contornoEsq.length) continue;
        menor = Math.min(menor, no.contornoEsq[k]! + dx);
        maior = Math.max(maior, no.contornoDir[k]! + dx);
      }

      contornoEsq.push(menor);
      contornoDir.push(maior);
    }

    return { contornoEsq, contornoDir };
  }

  /** Converte as coordenadas relativas em absolutas, num passo de cima para
   * baixo. É a segunda metade do algoritmo: a primeira acumula deslocamentos,
   * esta os soma. */
  private achatar(
    d: Disposto,
    x: number,
    nivel: number,
    saida: Map<number, { x: number; nivel: number }>,
  ): void {
    saida.set(d.id, { x, nivel });
    for (const { no, dx } of d.filhos) {
      this.achatar(no, x + dx, nivel + 1, saida);
    }
  }

  /** Nós que existem no modelo mas que a raiz não alcança. */
  private orfaos(
    m: Modelo,
    alvos: Map<number, { x: number; nivel: number }>,
  ): number[] {
    return m.ordem.filter((id) => !alvos.has(id));
  }

  /** Quanto vale uma unidade de layout, para a árvore caber na tela.
   *
   * A escala é limitada por cima para uma árvore de três nós não virar três
   * bolas gigantes, e por baixo pelo diâmetro do nó — abaixo disso os nós se
   * tocariam, e é melhor a árvore transbordar do que mentir sobre a forma. */
  private escalar(alvos: Map<number, { x: number; nivel: number }>): number {
    if (alvos.size === 0) return this.raio * 2.6;

    let menor = 0;
    let maior = 0;
    for (const pos of alvos.values()) {
      menor = Math.min(menor, pos.x);
      maior = Math.max(maior, pos.x);
    }

    const largura = maior - menor;
    const util = this.larguraCss - 2 * (MARGEM_X + this.raio);
    const cabe = largura > 0 ? util / largura : Infinity;

    return Math.max(this.raio * 2.15, Math.min(this.raio * 2.9, cabe));
  }

  /* ---- desenho --------------------------------------------------------- */

  private pintar(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;

    this.limpar();

    /* Arestas primeiro, para o nó cobrir a ponta da linha. */
    for (const id of m.ordem) {
      const pose = this.poses.get(id);
      if (!pose || !m.nos.has(id)) continue;

      filhosDe(m, id).forEach((destino, indice) => {
        const poseDestino = destino !== 0 ? this.poses.get(destino) : undefined;
        if (!poseDestino) return;

        const de = this.saidaDaAresta(m, id, indice, pose.x, pose.y);
        const para = this.entradaDaAresta(
          m,
          destino,
          poseDestino.x,
          poseDestino.y,
        );

        ctx.globalAlpha = Math.min(pose.alfa, poseDestino.alfa);
        ctx.strokeStyle = p.linha2;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(de.x, de.y);
        ctx.lineTo(para.x, para.y);
        ctx.stroke();
        ctx.globalAlpha = 1;
      });
    }

    for (const id of m.ordem) {
      if (!this.poses.has(id) || !m.nos.has(id)) continue;
      this.desenharNo(m, id);
    }

    this.ponteiroRaiz(m);
  }

  /** O rótulo `raiz`, que é o único ponteiro nomeado que uma árvore tem. */
  private ponteiroRaiz(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const alvo = alvoDe(m, Ptr.PTR_RAIZ);
    const pose = alvo !== 0 ? this.poses.get(alvo) : undefined;

    ctx.font = `600 12px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";

    if (!pose) {
      ctx.fillStyle = p.fg3;
      ctx.fillText("raiz = NULL", this.larguraCss / 2, MARGEM_TOPO);
      return;
    }

    ctx.fillStyle = p.acento;
    ctx.fillText("raiz", pose.x, pose.y - this.raio - 16);

    ctx.strokeStyle = p.acento;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(pose.x, pose.y - this.raio - 9);
    ctx.lineTo(pose.x, pose.y - this.raio - 2);
    ctx.stroke();
    this.setaBaixo(pose.x, pose.y - this.raio, p.acento);
  }

  /* ---- os três ganchos ------------------------------------------------- *
   *
   * São as únicas decisões que dependem da FORMA do nó, e é por isso que a
   * árvore B precisou sobrescrever exatamente estas três — e nada do layout.
   * Um nó-página é uma caixa com várias células; um nó-círculo tem uma chave
   * no meio. O resto é igual.                                              */

  /** Desenha um nó. O padrão é o círculo da ABB e da AVL. */
  protected desenharNo(m: Modelo, id: number): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const pose = this.poses.get(id);
    const no = m.nos.get(id);
    if (!pose || !no) return;

    const visitado = m.visitados.has(id);
    ctx.globalAlpha = pose.alfa;

    ctx.beginPath();
    ctx.arc(pose.x, pose.y, this.raio, 0, Math.PI * 2);
    ctx.fillStyle = p.bg2;
    ctx.fill();

    /* O acento é da interface, e o cursor do algoritmo é justamente onde a
     * interface está olhando — é a única vez que ele encosta num nó. */
    ctx.strokeStyle = visitado ? p.acento : p.linha2;
    ctx.lineWidth = visitado ? 2 : 1;
    ctx.stroke();

    /* A fonte acompanha o nó: com a árvore alta o raio encolhe, e um 14px
     * dentro de um círculo de 9 vazaria por todos os lados. */
    const corpo = Math.max(8, Math.round(this.raio * 0.74));

    ctx.fillStyle = p.fg;
    ctx.font = `600 ${corpo}px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(no.valor), pose.x, pose.y);

    /* O fator de balanceamento, quando a estrutura tem um.
     *
     * Fica fora do círculo, de propósito: ele não é a chave, é uma medida
     * sobre o nó. E acende quando estoura — o instante em que |FB| passa de 1
     * é a única explicação de por que a rotação vai acontecer NAQUELE nó e não
     * em outro. Sem esse número, ela parece mágica. */
    if (no.fb !== null) {
      const estourou = no.fb > 1 || no.fb < -1;

      ctx.fillStyle = estourou ? p.stSwap : p.fg3;
      ctx.font = `600 ${Math.max(8, corpo - 3)}px ${p.mono}`;
      ctx.textAlign = "left";
      ctx.textBaseline = "middle";
      ctx.fillText(
        no.fb > 0 ? `+${no.fb}` : String(no.fb),
        pose.x + this.raio + 4,
        pose.y - this.raio + 4,
      );
    }

    ctx.globalAlpha = 1;
  }

  /** De onde a aresta para o filho `indice` sai. Do fundo do círculo, aqui. */
  protected saidaDaAresta(
    m: Modelo,
    id: number,
    indice: number,
    x: number,
    y: number,
  ): { x: number; y: number } {
    void m;
    void id;
    void indice;
    return { x, y: y + this.raio };
  }

  /** Onde a aresta encosta no filho. No topo do círculo, aqui. */
  protected entradaDaAresta(
    m: Modelo,
    id: number,
    x: number,
    y: number,
  ): { x: number; y: number } {
    void m;
    void id;
    return { x, y: y - this.raio };
  }
}
