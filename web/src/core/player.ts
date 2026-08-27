/* web/src/core/player.ts — play, pause, passo, scrub e velocidade.
 *
 * O estado da animação vive aqui, fora do React. O React só recebe uma foto,
 * com throttle: redesenhar a árvore de componentes a 60 Hz para mexer um
 * contador é desperdício, e trava a animação em máquina modesta.
 *
 * Cada ds_call zera o trace do lado do C. A linha do tempo que dá para
 * arrastar é acumulada deste lado: é ela que deixa voltar cinco operações. */

import { aplicar } from "../model/aplicar";
import { modeloNovo, type Modelo } from "../model/modelo";
import type { Ev } from "./bridge";

/** Eventos por segundo em velocidade 1x. */
const RITMO_BASE = 6;

/** Frequência com que o React é avisado. Não 60. */
const HZ_REACT = 15;

export interface FotoPlayer {
  readonly i: number;
  readonly total: number;
  readonly tocando: boolean;
  readonly velocidade: number;
  /* Muda a cada linha do tempo nova.
   *
   * Sem isto, trocar de estrutura podia não avisar o React: duas sessões
   * diferentes emitem o mesmo número de eventos, então i e total ficavam
   * iguais e a foto era considerada inalterada — enquanto o canvas, que
   * desenha por quadro, já mostrava a estrutura nova. Os painéis ficavam
   * mostrando os números da anterior. */
  readonly geracao: number;
}

/** Os eventos que uma operação emitiu, uma lista por trilha. */
export type Operacao = Ev[][];

/* Um passo da linha do tempo: o que cada trilha faz neste instante.
 *
 * null é "esta trilha já acabou a sua parte da operação e espera a outra". É
 * daí que sai a sincronia do modo comparar: as duas implementações começam
 * cada operação juntas, e a que precisa de menos eventos fica parada até a
 * outra terminar. Sincronizar por evento não daria certo — o mesmo push emite
 * cinco eventos na encadeada e quatro na de vetor, e as trilhas iam se
 * desencontrando um pouco a cada operação. */
type Quadro = Array<Ev | null>;

/** Enfileira as operações em quadros, alinhando as trilhas em cada uma. */
function emQuadros(ops: Operacao[], trilhas: number): Quadro[] {
  const quadros: Quadro[] = [];

  for (const op of ops) {
    let passos = 0;
    for (const eventos of op) passos = Math.max(passos, eventos.length);

    for (let i = 0; i < passos; i++) {
      const quadro: Quadro = new Array<Ev | null>(trilhas).fill(null);
      for (let t = 0; t < trilhas; t++) quadro[t] = op[t]?.[i] ?? null;
      quadros.push(quadro);
    }
  }

  return quadros;
}

export class Player {
  private quadros: Quadro[] = [];
  private modelos: Modelo[] = [modeloNovo()];
  private i = 0;
  private vel = 1;
  private tocando = false;

  private acumulado = 0;
  private instanteAnterior = 0;
  private raf = 0;

  private assinantes = new Set<() => void>();
  private porQuadro = new Set<() => void>();
  private avisoPendente = 0;
  private geracao = 0;
  private foto: FotoPlayer = {
    i: 0,
    total: 0,
    tocando: false,
    velocidade: 1,
    geracao: 0,
  };

  constructor() {
    this.laco = this.laco.bind(this);
  }

  iniciarLaco(): () => void {
    if (this.raf === 0) {
      this.instanteAnterior = performance.now();
      this.raf = requestAnimationFrame(this.laco);
    }
    return () => {
      cancelAnimationFrame(this.raf);
      this.raf = 0;
    };
  }

  /* ---- leitura ------------------------------------------------------- */

  get estado(): Modelo {
    return this.modelos[0]!;
  }

  /** Quantas trilhas a linha do tempo atual tem. */
  get trilhas(): number {
    return this.modelos.length;
  }

  estadoDe(trilha: number): Modelo {
    return this.modelos[trilha] ?? this.modelos[0]!;
  }

  /** Eventos já aplicados numa trilha, do mais recente para o mais antigo.
   *
   * Os quadros vazios da trilha são pulados: o log mostra o que ela fez, e
   * esperar a outra não é uma coisa que ela fez. */
  historico(limite: number, trilha = 0): Ev[] {
    const saida: Ev[] = [];

    for (let j = this.i - 1; j >= 0 && saida.length < limite; j--) {
      const ev = this.quadros[j]?.[trilha];
      if (ev) saida.push(ev);
    }
    return saida;
  }

  ler = (): FotoPlayer => this.foto;

  assinar = (fn: () => void): (() => void) => {
    this.assinantes.add(fn);
    return () => this.assinantes.delete(fn);
  };

  /** Chamado a cada quadro. É por aqui que o renderizador desenha. */
  aoQuadro(fn: () => void): () => void {
    this.porQuadro.add(fn);
    return () => this.porQuadro.delete(fn);
  }

  /* ---- linha do tempo ------------------------------------------------- */

  /** Troca a linha do tempo inteira.
   *
   * Para no FIM, não no começo. Os eventos que uma sessão nova emite são a
   * existência da estrutura — o vetor com a sua capacidade, os ponteiros
   * apontando para lugar nenhum. Isso é estado inicial, não operação para
   * assistir: parar antes deles deixaria a tela vazia sem explicação. */
  carregar(evs: Ev[]): void {
    this.carregarTrilhas([[evs]]);
  }

  /** A mesma coisa, com mais de uma trilha: o modo comparar. */
  carregarTrilhas(ops: Operacao[]): void {
    let trilhas = 1;
    for (const op of ops) trilhas = Math.max(trilhas, op.length);

    this.quadros = emQuadros(ops, trilhas);
    this.modelos = Array.from({ length: trilhas }, modeloNovo);
    this.geracao++;
    this.i = 0;
    this.tocando = false;
    this.irPara(this.quadros.length);
  }

  /** Acrescenta o trace de uma operação nova e a mostra acontecendo.
   *
   * A linha do tempo nunca é truncada. A estrutura que o C tem reflete todas
   * as operações já executadas, esteja o cursor onde estiver — onde a
   * animação parou é assunto da vista. Descartar eventos aqui apagaria
   * operações que de fato aconteceram, e foi exatamente o que acontecia ao
   * operar antes de a animação anterior terminar. */
  anexar(evs: Ev[]): void {
    this.anexarTrilhas([[evs]]);
  }

  /** Acrescenta operações já alinhadas entre as trilhas. */
  anexarTrilhas(ops: Operacao[]): void {
    const inicioDosNovos = this.quadros.length;
    const novos = emQuadros(ops, this.trilhas);

    this.quadros.push(...novos);

    /* Volta ao instante anterior aos eventos novos, para eles serem vistos
     * acontecendo em vez de aparecerem prontos. */
    this.irPara(inicioDosNovos);
    this.tocando = novos.length > 0;
    this.avisar();
  }

  /** Vai ao passo k. Voltar reexecuta de 0 — nunca desfaz evento.
   *
   * Ir para a frente é só continuar aplicando, porque o modelo já é
   * exatamente o estado do passo atual. Voltar não tem esse atalho: é aí
   * que a reexecução paga o seu preço, e é o que dispensa escrever o
   * inverso de cada evento. */
  irPara(k: number): void {
    const destino = Math.max(0, Math.min(k, this.quadros.length));
    let de = this.i;

    if (destino < this.i) {
      this.modelos = this.modelos.map(modeloNovo);
      de = 0;
    }

    for (let j = de; j < destino; j++) {
      this.aplicarQuadro(j);
    }

    this.i = destino;
    this.acumulado = 0;
    this.avisar();
  }

  passo(delta: number): void {
    this.pause();
    this.irPara(this.i + delta);
  }

  play(): void {
    if (this.i >= this.quadros.length) {
      this.irPara(0);
    }
    this.tocando = true;
    this.acumulado = 0;
    this.avisar();
  }

  pause(): void {
    if (this.tocando) {
      this.tocando = false;
      this.avisar();
    }
  }

  alternar(): void {
    if (this.tocando) this.pause();
    else this.play();
  }

  setVelocidade(v: number): void {
    this.vel = v;
    this.avisar();
  }

  /* ---- laço ----------------------------------------------------------- */

  private laco(agora: number): void {
    const dt = Math.min((agora - this.instanteAnterior) / 1000, 0.25);
    this.instanteAnterior = agora;

    if (this.tocando) {
      this.acumulado += dt * RITMO_BASE * this.vel;
      while (this.acumulado >= 1 && this.i < this.quadros.length) {
        this.aplicarQuadro(this.i);
        this.i++;
        this.acumulado -= 1;
      }
      if (this.i >= this.quadros.length) {
        this.tocando = false;
        this.acumulado = 0;
      }
      this.avisar();
    }

    for (const fn of this.porQuadro) fn();

    this.raf = requestAnimationFrame(this.laco);
  }

  /** Aplica o quadro j em cada trilha que tenha evento nele. */
  private aplicarQuadro(j: number): void {
    const quadro = this.quadros[j];
    if (!quadro) return;

    for (let t = 0; t < quadro.length; t++) {
      const ev = quadro[t];
      const modelo = this.modelos[t];
      if (ev && modelo) aplicar(modelo, ev);
    }
  }

  private avisar(): void {
    const mudou =
      this.foto.i !== this.i ||
      this.foto.total !== this.quadros.length ||
      this.foto.tocando !== this.tocando ||
      this.foto.velocidade !== this.vel ||
      this.foto.geracao !== this.geracao;

    if (!mudou) return;

    /* A foto é sempre atualizada. Antes ela ficava presa junto com o aviso, e
     * quem lesse logo depois de uma operação via dados velhos — o throttle é
     * sobre com que frequência o React redesenha, não sobre a verdade. */
    this.foto = {
      i: this.i,
      total: this.quadros.length,
      tocando: this.tocando,
      velocidade: this.vel,
      geracao: this.geracao,
    };

    /* Durante o play o índice muda várias vezes por segundo, e o painel não
     * precisa acompanhar quadro a quadro. Nenhum aviso se perde: o laço chama
     * isto a cada quadro, e o fim da reprodução zera `tocando`, que passa
     * direto pelo throttle. */
    const agora = performance.now();
    if (this.tocando && agora - this.avisoPendente < 1000 / HZ_REACT) return;
    this.avisoPendente = agora;

    for (const fn of this.assinantes) fn();
  }
}
