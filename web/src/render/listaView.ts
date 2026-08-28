/* web/src/render/listaView.ts — a lista deitada, com quebra de linha.
 *
 * A cadeia anda para a direita e quebra quando não cabe, como o plano pede. É
 * uma orientação diferente da pilha por um motivo concreto: numa lista o que
 * interessa é a POSIÇÃO, e posição se lê da esquerda para a direita.
 *
 * Três coisas aqui não existem no grafo vertical, e são as três que distinguem
 * as implementações na tela:
 *
 *   - a seta de volta (`ant`), que só a lista dupla tem;
 *   - o arco do fim para o começo, que só a circular tem;
 *   - o cursor, que mostra a caminhada até a posição pedida — e é a razão de a
 *     lista simples parecer lenta ao lado da dupla. */

import { Ptr, Tipo } from "../core/ops";
import { alvoDe, type Modelo } from "../model/modelo";
import { Tela } from "./tela";

const LARGURA_NO = 92;
const ALTURA_NO = 44;
const VAO_X = 40; /* espaço entre um nó e o seguinte, onde a seta cabe */
const VAO_Y = 78; /* entre uma linha e a de baixo, onde a volta cabe */
const MARGEM_X = 28;
const MARGEM_TOPO = 74; /* o rótulo do ponteiro fica acima do da posição */
const RAIO = 7;

const SLOT_PROX = 0;
const SLOT_ANT = 1;

export class ListaView extends Tela {
  constructor(
    canvas: HTMLCanvasElement,
    private tipo: number,
  ) {
    super(canvas);
  }

  /* ---- layout --------------------------------------------------------- */

  private ehDupla(): boolean {
    return this.tipo === Tipo.TIPO_LISTA_DUPLA;
  }

  private ehCircular(): boolean {
    return this.tipo === Tipo.TIPO_LISTA_CIRCULAR;
  }

  /** Quantos nós cabem numa linha. Pelo menos um, mesmo em canvas estreito. */
  private porLinha(): number {
    const util = this.larguraCss - 2 * MARGEM_X;
    return Math.max(1, Math.floor((util + VAO_X) / (LARGURA_NO + VAO_X)));
  }

  private posicaoDe(indice: number): { x: number; y: number } {
    const porLinha = this.porLinha();
    const linha = Math.floor(indice / porLinha);
    const coluna = indice % porLinha;

    return {
      x: MARGEM_X + coluna * (LARGURA_NO + VAO_X) + LARGURA_NO / 2,
      y: MARGEM_TOPO + linha * (ALTURA_NO + VAO_Y),
    };
  }

  /** O primeiro nó da lista.
   *
   * Na circular não existe ponteiro de início: a estrutura guarda só o último,
   * e o primeiro é o sucessor dele. Fazer essa conta aqui é o que dispensa o C
   * de emitir um ponteiro que ele não tem. */
  private primeiro(m: Modelo): number {
    if (!this.ehCircular()) {
      return alvoDe(m, Ptr.PTR_INICIO);
    }
    const ultimo = alvoDe(m, Ptr.PTR_FIM);
    return ultimo !== 0 ? (m.nos.get(ultimo)?.arestas.get(SLOT_PROX) ?? 0) : 0;
  }

  /** A cadeia, seguindo prox. O conjunto de vistos é o que segura a circular:
   * sem ele o laço aqui daria a mesma volta infinita que no C. */
  private cadeia(m: Modelo): number[] {
    const ordem: number[] = [];
    const visto = new Set<number>();
    let id = this.primeiro(m);

    while (id !== 0 && m.nos.has(id) && !visto.has(id)) {
      visto.add(id);
      ordem.push(id);
      id = m.nos.get(id)?.arestas.get(SLOT_PROX) ?? 0;
    }

    /* O nó recém-criado, antes de ser ligado, entra no fim — que na lista é
     * onde ele visualmente não atrapalha, ao contrário da pilha. Escondê-lo
     * tornaria um bug de trace invisível. */
    const orfaos = m.ordem.filter((x) => !visto.has(x));
    return [...ordem, ...orfaos];
  }

  /* ---- quadro --------------------------------------------------------- */

  desenhar(m: Modelo): void {
    const ordem = this.cadeia(m);

    ordem.forEach((id, indice) => {
      const { x, y } = this.posicaoDe(indice);

      /* Nasce um pouco abaixo do lugar: o nó novo sobe para a fila em vez de
       * aparecer pronto, e é isso que deixa ver ONDE ele entrou. */
      this.mirar(id, x, y, {
        x,
        y: y + 26,
        alvoX: x,
        alvoY: y,
        alfa: 0,
        alvoAlfa: 1,
      });
    });

    this.animar((id) => m.nos.has(id));
    this.pintar(m, ordem);
  }

  private pintar(m: Modelo, ordem: number[]): void {
    this.limpar();

    this.arestas(m, ordem);
    if (this.ehCircular()) this.voltaCircular(ordem);
    this.nos(m, ordem);
    this.ponteiros(m);
    this.cursor(m);
  }

  /* ---- arestas --------------------------------------------------------- */

  private arestas(m: Modelo, ordem: number[]): void {
    const ctx = this.ctx;
    const p = this.paleta;

    for (const id of ordem) {
      const pose = this.poses.get(id);
      if (!pose) continue;

      const destino = m.nos.get(id)?.arestas.get(SLOT_PROX) ?? 0;
      const poseDestino = destino !== 0 ? this.poses.get(destino) : undefined;

      ctx.globalAlpha = pose.alfa;

      /* Na circular, a ligação do último para o primeiro tem desenho próprio:
       * o arco. Aqui ela seria uma seta atravessando a lista inteira. */
      const ehVolta =
        this.ehCircular() && destino !== 0 && destino === ordem[0] && id !== destino;

      if (!poseDestino) {
        this.nulo(pose.x + LARGURA_NO / 2, pose.y);
      } else if (!ehVolta) {
        /* A seta de volta é desenhada a partir da aresta que o modelo tem, e
         * não da vizinhança na tela. Assim um `ant` que o C esquecesse de
         * ligar aparece como seta faltando — o invariante da lista dupla
         * virando desenho, em vez de um enfeite que sempre bate. */
        const voltaLigada =
          m.nos.get(destino)?.arestas.get(SLOT_ANT) === id;
        this.ligacao(pose, poseDestino, this.ehDupla() && voltaLigada);
      }

      ctx.globalAlpha = 1;
    }

    ctx.strokeStyle = p.linha2;
  }

  /** A seta de a para b — reta quando são vizinhos na mesma linha, e descendo
   * pela borda quando a linha quebrou. */
  private ligacao(
    a: { x: number; y: number },
    b: { x: number; y: number },
    dupla: boolean,
  ): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const mesmaLinha = Math.abs(a.y - b.y) < 1 && b.x > a.x;
    /* Descer pela borda só faz sentido indo para a linha DE BAIXO. Sem esta
     * condição o caminho degenerava numa reta atravessando a tela inteira
     * sempre que o destino ficava à esquerda — o que acontece a cada inserção
     * no início, enquanto o nó novo ainda viaja para o lugar dele. */
    const proximaLinha = b.y > a.y + 1;

    ctx.strokeStyle = p.linha2;
    ctx.lineWidth = 1.5;

    if (mesmaLinha) {
      const x0 = a.x + LARGURA_NO / 2;
      const x1 = b.x - LARGURA_NO / 2;
      /* Com dois sentidos, cada seta ganha a sua altura: sobrepostas elas
       * viravam uma linha só e a lista dupla parecia simples. */
      const yProx = dupla ? a.y - 7 : a.y;

      ctx.beginPath();
      ctx.moveTo(x0, yProx);
      ctx.lineTo(x1, yProx);
      ctx.stroke();
      this.setaDireita(x1, yProx, p.linha2);

      if (dupla) {
        const yAnt = a.y + 7;
        ctx.beginPath();
        ctx.moveTo(x1, yAnt);
        ctx.lineTo(x0, yAnt);
        ctx.stroke();
        this.setaEsquerda(x0, yAnt, p.linha2);
      }
      return;
    }

    if (proximaLinha) {
      /* Quebra de linha: sai pela direita, desce, e entra pela esquerda de
       * baixo. É o mesmo caminho que o olho faz ao ler. */
      const meio = (a.y + b.y) / 2;
      ctx.beginPath();
      ctx.moveTo(a.x + LARGURA_NO / 2, a.y);
      ctx.lineTo(this.larguraCss - 10, a.y);
      ctx.lineTo(this.larguraCss - 10, meio);
      ctx.lineTo(10, meio);
      ctx.lineTo(10, b.y);
      ctx.lineTo(b.x - LARGURA_NO / 2, b.y);
      ctx.stroke();
      this.setaDireita(b.x - LARGURA_NO / 2, b.y, p.linha2);
      return;
    }

    /* Qualquer outro arranjo — destino à esquerda, ou acima — é estado de
     * passagem, enquanto um nó ainda vai para o lugar dele. Uma reta entre os
     * dois é o desenho honesto: mostra a ligação sem inventar um caminho. */
    const dx = b.x - a.x;
    const borda = dx >= 0 ? LARGURA_NO / 2 : -LARGURA_NO / 2;
    ctx.beginPath();
    ctx.moveTo(a.x + borda, a.y);
    ctx.lineTo(b.x - borda, b.y);
    ctx.stroke();
    if (dx >= 0) {
      this.setaDireita(b.x - borda, b.y, p.linha2);
    } else {
      this.setaEsquerda(b.x - borda, b.y, p.linha2);
    }
  }

  /** O arco do último nó de volta ao primeiro, por baixo de tudo. */
  private voltaCircular(ordem: number[]): void {
    const ctx = this.ctx;
    const p = this.paleta;

    const primeiro = ordem[0] !== undefined ? this.poses.get(ordem[0]) : undefined;
    const ultimo =
      ordem.length > 0 ? this.poses.get(ordem[ordem.length - 1]!) : undefined;
    if (!primeiro || !ultimo) return;

    /* Um nó sozinho aponta para si mesmo: uma alça, e não um arco. */
    const sozinho = ordem.length === 1;
    let fundo = MARGEM_TOPO;
    for (const pose of this.poses.values()) fundo = Math.max(fundo, pose.y);
    /* Abaixo do id do nó e do rótulo do cursor, que moram os dois logo sob a
     * fila de nós — senão o arco passa por cima deles. */
    fundo += ALTURA_NO / 2 + 50;

    ctx.save();
    ctx.globalAlpha = Math.min(primeiro.alfa, ultimo.alfa);
    ctx.strokeStyle = p.linha2;
    ctx.lineWidth = 1.5;
    /* Tracejado porque é a ligação que fecha o ciclo, e não mais um passo da
     * cadeia: o traço já a distingue antes de a cor entrar. */
    ctx.setLineDash([5, 4]);

    ctx.beginPath();
    if (sozinho) {
      ctx.moveTo(primeiro.x + LARGURA_NO / 2, primeiro.y);
      ctx.lineTo(primeiro.x + LARGURA_NO / 2 + 22, primeiro.y);
      ctx.lineTo(primeiro.x + LARGURA_NO / 2 + 22, fundo);
      ctx.lineTo(primeiro.x, fundo);
      ctx.lineTo(primeiro.x, primeiro.y + ALTURA_NO / 2);
    } else {
      ctx.moveTo(ultimo.x, ultimo.y + ALTURA_NO / 2);
      ctx.lineTo(ultimo.x, fundo);
      ctx.lineTo(primeiro.x, fundo);
      ctx.lineTo(primeiro.x, primeiro.y + ALTURA_NO / 2);
    }
    ctx.stroke();
    ctx.restore();

    ctx.globalAlpha = Math.min(primeiro.alfa, ultimo.alfa);
    this.setaCima(primeiro.x, primeiro.y + ALTURA_NO / 2, p.linha2);
    ctx.globalAlpha = 1;
  }

  private nulo(x: number, y: number): void {
    const ctx = this.ctx;
    const p = this.paleta;

    ctx.strokeStyle = p.linha2;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(x, y);
    ctx.lineTo(x + 16, y);
    ctx.stroke();

    ctx.fillStyle = p.fg3;
    ctx.font = `500 11px ${p.mono}`;
    ctx.textAlign = "left";
    ctx.textBaseline = "middle";
    ctx.fillText("NULL", x + 20, y);
  }

  /* ---- nós ------------------------------------------------------------- */

  private nos(m: Modelo, ordem: number[]): void {
    const ctx = this.ctx;
    const p = this.paleta;

    ordem.forEach((id, indice) => {
      const pose = this.poses.get(id);
      const no = m.nos.get(id);
      if (!pose || !no) return;

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
      ctx.strokeStyle = m.visitados.has(id) ? p.acento : p.linha2;
      ctx.lineWidth = m.visitados.has(id) ? 2 : 1;
      ctx.stroke();

      ctx.fillStyle = p.fg;
      ctx.font = `600 16px ${p.mono}`;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(String(no.valor), pose.x, pose.y);

      /* A posição por cima do nó, o id por baixo. A posição é o argumento das
       * operações, então ela é o número que se procura na tela; o id só serve
       * para casar o desenho com o trace na hora de depurar. */
      ctx.fillStyle = p.acento;
      ctx.font = `600 10px ${p.mono}`;
      ctx.textBaseline = "bottom";
      ctx.fillText(String(indice), pose.x, pose.y - ALTURA_NO / 2 - 5);

      ctx.fillStyle = p.fg3;
      ctx.font = `500 9px ${p.mono}`;
      ctx.textBaseline = "top";
      ctx.fillText(`#${id}`, pose.x, pose.y + ALTURA_NO / 2 + 4);

      ctx.globalAlpha = 1;
    });
  }

  /* ---- ponteiros ------------------------------------------------------- */

  private rotulos(): Array<{ ptr: number; texto: string }> {
    if (this.ehCircular()) {
      /* Um ponteiro só, e é o do fim. É a estrutura inteira num rótulo. */
      return [{ ptr: Ptr.PTR_FIM, texto: "fim" }];
    }
    return this.ehDupla()
      ? [
          { ptr: Ptr.PTR_INICIO, texto: "início" },
          { ptr: Ptr.PTR_FIM, texto: "fim" },
        ]
      : [{ ptr: Ptr.PTR_INICIO, texto: "início" }];
  }

  /** Os ponteiros da estrutura, pendurados acima do nó que apontam. */
  private ponteiros(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const porAlvo = new Map<number, string[]>();

    for (const rotulo of this.rotulos()) {
      const alvo = alvoDe(m, rotulo.ptr);
      porAlvo.set(alvo, [...(porAlvo.get(alvo) ?? []), rotulo.texto]);
    }

    for (const [alvo, textos] of porAlvo) {
      const texto = textos.join(" · ");
      const pose = alvo !== 0 ? this.poses.get(alvo) : undefined;

      ctx.font = `600 11px ${p.mono}`;
      ctx.textAlign = pose ? "center" : "left";
      ctx.textBaseline = "bottom";

      if (!pose) {
        /* Lista vazia: o ponteiro existe e não aponta para nada. Dizê-lo é
         * mais informativo do que uma tela em branco. */
        ctx.fillStyle = p.fg3;
        ctx.fillText(`${texto} = NULL`, MARGEM_X, MARGEM_TOPO - 26);
        continue;
      }

      /* O rótulo fica acima do número da posição, e a seta desce encostada na
       * borda esquerda do nó. Centralizada, ela passava exatamente por cima da
       * posição — que é o número que se procura na tela numa lista. */
      const xSeta = pose.x - LARGURA_NO / 2 + 11;
      const y = pose.y - ALTURA_NO / 2 - 22;

      ctx.globalAlpha = pose.alfa;
      ctx.fillStyle = p.acento;
      ctx.fillText(texto, xSeta, y);

      ctx.strokeStyle = p.acento;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(xSeta, y + 3);
      ctx.lineTo(xSeta, pose.y - ALTURA_NO / 2 - 2);
      ctx.stroke();
      this.setaBaixo(xSeta, pose.y - ALTURA_NO / 2 - 2, p.acento);
      ctx.globalAlpha = 1;
    }
  }

  /** O cursor da travessia, por baixo do nó.
   *
   * Fica embaixo justamente para não disputar espaço com início e fim, que
   * estão em cima: durante uma caminhada os três aparecem ao mesmo tempo, e é
   * o cursor que se move. */
  private cursor(m: Modelo): void {
    const ctx = this.ctx;
    const p = this.paleta;
    const alvo = alvoDe(m, Ptr.PTR_CURSOR);
    const pose = alvo !== 0 ? this.poses.get(alvo) : undefined;

    if (!pose) return;

    const y = pose.y + ALTURA_NO / 2 + 20;

    ctx.globalAlpha = pose.alfa;
    ctx.strokeStyle = p.acento;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(pose.x, y);
    ctx.lineTo(pose.x, pose.y + ALTURA_NO / 2 + 2);
    ctx.stroke();
    this.setaCima(pose.x, pose.y + ALTURA_NO / 2 + 2, p.acento);

    ctx.fillStyle = p.acento;
    ctx.font = `600 11px ${p.mono}`;
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.fillText("cursor", pose.x, y + 2);
    ctx.globalAlpha = 1;
  }
}
