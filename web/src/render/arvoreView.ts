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
import { alvoDe, type Modelo } from "../model/modelo";
import { Tela } from "./tela";

const SLOT_ESQ = 0;
const SLOT_DIR = 1;

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
  private raio = RAIO_MAX;
  private passoY = NIVEL_MAX;

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

  /** Dispõe a subárvore de `id`, em coordenadas relativas à própria raiz. */
  private dispor(m: Modelo, id: number, visto: Set<number>): Disposto | null {
    /* `visto` protege contra um ciclo vindo de um trace defeituoso. Uma ABB
     * não tem ciclos; um bug de EV_EDGE_SET, sim, e travar a aba inteira num
     * laço infinito seria o pior jeito de descobrir isso. */
    if (id === 0 || !m.nos.has(id) || visto.has(id)) return null;
    visto.add(id);

    const no = m.nos.get(id)!;
    const esq = this.dispor(m, no.arestas.get(SLOT_ESQ) ?? 0, visto);
    const dir = this.dispor(m, no.arestas.get(SLOT_DIR) ?? 0, visto);

    const filhos: Disposto["filhos"] = [];

    if (esq && dir) {
      /* O aperto: para cada nível em que os dois existem, quanto o de baixo
       * teria que andar para não encostar no de cima. O maior deles manda. */
      let afastamento = SEPARACAO;
      const comuns = Math.min(esq.contornoDir.length, dir.contornoEsq.length);

      for (let k = 0; k < comuns; k++) {
        const aperto =
          esq.contornoDir[k]! - dir.contornoEsq[k]! + SEPARACAO;
        if (aperto > afastamento) afastamento = aperto;
      }

      filhos.push({ no: esq, dx: -afastamento / 2 });
      filhos.push({ no: dir, dx: +afastamento / 2 });
    } else if (esq) {
      /* Meio passo para a direita do filho: é o desvio deliberado do
       * algoritmo original, e é o que faz a árvore degenerada parecer
       * degenerada. */
      filhos.push({ no: esq, dx: -SEPARACAO / 2 });
    } else if (dir) {
      filhos.push({ no: dir, dx: +SEPARACAO / 2 });
    }

    return { id, filhos, ...this.contornos(filhos) };
  }

  /** Os dois perfis da subárvore, nível a nível, a partir dos filhos. */
  private contornos(filhos: Disposto["filhos"]): {
    contornoEsq: number[];
    contornoDir: number[];
  } {
    const contornoEsq = [0];
    const contornoDir = [0];

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
      const no = m.nos.get(id);
      if (!pose || !no) continue;

      for (const slot of [SLOT_ESQ, SLOT_DIR]) {
        const destino = no.arestas.get(slot) ?? 0;
        const poseDestino = destino !== 0 ? this.poses.get(destino) : undefined;
        if (!poseDestino) continue;

        ctx.globalAlpha = Math.min(pose.alfa, poseDestino.alfa);
        ctx.strokeStyle = p.linha2;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(pose.x, pose.y + this.raio);
        ctx.lineTo(poseDestino.x, poseDestino.y - this.raio);
        ctx.stroke();
        ctx.globalAlpha = 1;
      }
    }

    for (const id of m.ordem) {
      const pose = this.poses.get(id);
      const no = m.nos.get(id);
      if (!pose || !no) continue;

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
       * sobre o nó. E acende quando estoura — o instante em que |FB| passa de
       * 1 é a única explicação de por que a rotação vai acontecer NAQUELE nó
       * e não em outro. Sem esse número, ela parece mágica. */
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
}
