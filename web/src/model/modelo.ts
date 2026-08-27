/* web/src/model/modelo.ts — o estado visual, reconstruído a partir dos eventos.
 *
 * O C é a fonte da verdade lógica; isto aqui é a fonte da verdade visual. O
 * modelo é puramente lógico: não guarda coordenada nem cor. Onde cada nó é
 * desenhado é problema do renderizador, e é ele que faz o tween.
 *
 * Nada aqui desfaz evento. Para ir ao passo k, zera-se e aplicam-se os
 * eventos 0..k — implementar o inverso de cada evento seria trabalho dobrado
 * e fonte de bugs sutis. */

export interface NoModelo {
  readonly id: number;
  valor: number;
  /** slot -> id do destino. 0 é NULL, que é o que o C manda para ponteiro nulo. */
  arestas: Map<number, number>;
  /** Fator de balanceamento, só na AVL. null em quem não tem.
   *
   * Não é dado da estrutura no sentido em que a chave é — ele é derivado das
   * alturas —, mas é o número que a aula da AVL gira em torno: sem ele na
   * tela, a rotação parece acontecer sem motivo. */
  fb: number | null;
  /** As chaves de um nó de árvore B, em ordem.
   *
   * Vazio em toda estrutura de uma chave por nó — nelas o valor mora em
   * `valor`, e duplicá-lo aqui daria duas verdades sobre a mesma coisa. */
  chaves: number[];
  /** A página de disco em que o nó mora, ou null em quem vive na memória. */
  pagina: number | null;
}

/* O mundo "vetor": ordenação, busca e as implementações com arranjo.
 *
 * Destaque de leitura e escrita é o ÚLTIMO índice tocado, não um conjunto que
 * cresce. Conjunto acumularia ao reproduzir, e depois de arrastar a barra até
 * o fim a tela ficaria toda acesa. */
export interface VetorModelo {
  capacidade: number;
  /* null é célula nunca escrita — diferente de célula liberada, que tem
   * valor antigo e marca TAG_LIVRE. */
  valores: (number | null)[];
  marcas: number[];
  ultimoLido: number;
  ultimoEscrito: number;

  /* ---- o que a ordenação acrescentou ---------------------------------- *
   *
   * Tudo aqui é o ÚLTIMO de cada coisa, pela mesma razão que ultimoLido:
   * conjunto que cresce acumula ao reproduzir, e depois de arrastar a barra
   * até o fim a tela ficaria toda acesa.                                  */

  /** As duas células sendo comparadas agora, ou null. */
  comparando: readonly [number, number] | null;
  /** Verdadeiro quando o segundo operando é o valor em mãos, e não a célula
   * `b` do vetor. É o `c = 1` de EV_ARR_COMPARE. */
  comparandoMao: boolean;
  /** O subvetor que o algoritmo está olhando: a partição, a metade do merge,
   * o trecho ainda desordenado da bolha. */
  faixa: readonly [number, number] | null;
  /** O buffer auxiliar. Uma célula na inserção e no shell — o valor em mãos;
   * n células no merge. */
  aux: (number | null)[] | null;
  auxUltimoEscrito: number;
}

/** O que o algoritmo está fazendo agora: EV_PHASE, com os operandos. */
export interface FaseModelo {
  /** STR_* — a frase vem do i18n, como toda mensagem do C. */
  str: number;
  a: number;
  b: number;
}

export interface Modelo {
  nos: Map<number, NoModelo>;
  /** Ordem de criação. Dá ao layout uma ordem estável, independente do Map. */
  ordem: number[];
  /** PTR_* -> id de nó (encadeada) ou índice (vetor). */
  ponteiros: Map<number, number>;
  /** CNT_* -> valor acumulado. */
  contadores: Map<number, number>;
  /** Nós sob o cursor do algoritmo. */
  visitados: Set<number>;
  /** Último STR_* emitido, ou null. */
  mensagem: number | null;
  /** Último ponto do código-fonte que executou. É o que o painel destaca. */
  fonte: { src: number; linha: number } | null;
  /** Existe só depois de um EV_ARR_INIT. */
  vetor: VetorModelo | null;
  /** Existe só depois de um EV_PHASE. */
  fase: FaseModelo | null;
}

export function modeloNovo(): Modelo {
  return {
    nos: new Map(),
    ordem: [],
    ponteiros: new Map(),
    contadores: new Map(),
    visitados: new Set(),
    mensagem: null,
    fonte: null,
    vetor: null,
    fase: null,
  };
}

/** Alvo de um ponteiro nomeado, ou 0 se ele aponta para NULL. */
export function alvoDe(m: Modelo, ptr: number): number {
  return m.ponteiros.get(ptr) ?? 0;
}

export function contador(m: Modelo, cnt: number): number {
  return m.contadores.get(cnt) ?? 0;
}

/** Os filhos de um nó, na ordem dos slots.
 *
 * Numa árvore binária são dois — os slots 0 e 1 —, e numa árvore B são
 * `chaves.length + 1`. É a única diferença de forma entre as duas, e isolá-la
 * aqui é o que deixa o layout de Reingold–Tilford servir às duas sem saber
 * qual está desenhando.
 *
 * O 0 é NULL, como em toda aresta do projeto.
 */
export function filhosDe(m: Modelo, id: number): number[] {
  const no = m.nos.get(id);
  if (!no) return [];

  const quantos = no.chaves.length > 0 ? no.chaves.length + 1 : 2;
  const saida: number[] = [];

  for (let slot = 0; slot < quantos; slot++) {
    saida.push(no.arestas.get(slot) ?? 0);
  }
  return saida;
}

/**
 * A altura da árvore, medida no desenho.
 *
 * Não vem do C de propósito. O painel tem que dizer a altura da árvore que
 * está na tela, e a do C é a de depois que a operação inteira terminou —
 * arrastar a barra do transporte para o meio faria os dois discordarem.
 * Calculada aqui, o número e o desenho não têm como divergir.
 *
 * `visto` protege contra um ciclo vindo de um trace defeituoso: uma árvore não
 * tem ciclos, mas um EV_EDGE_SET errado, sim.
 */
export function alturaDaArvore(m: Modelo, raiz: number): number {
  const visto = new Set<number>();

  function medir(id: number): number {
    if (id === 0 || !m.nos.has(id) || visto.has(id)) return 0;
    visto.add(id);

    let abaixo = 0;
    for (const filho of filhosDe(m, id)) {
      abaixo = Math.max(abaixo, medir(filho));
    }
    return 1 + abaixo;
  }

  return medir(raiz);
}

/**
 * A maior cadeia de um hash encadeado, medida no desenho.
 *
 * Pelo mesmo motivo da altura da árvore: o número do painel tem que descrever
 * a tabela que está na tela, e não a de depois que a operação terminou. É a
 * medida de qualidade da função hash — numa tabela bem espalhada ele fica
 * perto de n/m, e numa mal escolhida ele denuncia o balde que virou lista.
 *
 * A célula do balde guarda o ID do nó da cabeça, e 0 é balde vazio: é o mesmo
 * significado que o 0 tem em toda aresta do projeto.
 */
export function maiorCadeia(m: Modelo): number {
  if (!m.vetor) return 0;

  let maior = 0;

  for (let b = 0; b < m.vetor.capacidade; b++) {
    const visto = new Set<number>();
    let id = m.vetor.valores[b] ?? 0;
    let quantos = 0;

    /* `visto` protege contra um ciclo vindo de um trace defeituoso: uma cadeia
     * não tem ciclos, mas um EV_EDGE_SET errado, sim. */
    while (id !== 0 && m.nos.has(id) && !visto.has(id)) {
      visto.add(id);
      quantos++;
      id = m.nos.get(id)?.arestas.get(0) ?? 0;
    }

    if (quantos > maior) maior = quantos;
  }

  return maior;
}

/**
 * Quantas células estão marcadas como túmulo num hash aberto.
 *
 * É a dívida que a remoção deixa: a célula saiu da tabela mas a sondagem
 * continua tendo que atravessá-la. É o número que explica por que uma tabela
 * aberta piora com o uso mesmo sem crescer.
 */
export function tumulos(m: Modelo, tagLivre: number): number {
  if (!m.vetor) return 0;
  return m.vetor.marcas.reduce((n, marca) => n + (marca === tagLivre ? 1 : 0), 0);
}

/**
 * A ordem lógica de uma fila circular: os índices físicos, a partir da frente,
 * dando a volta.
 *
 * É o que permite mostrar "1º, 2º, 3º" por cima das células enquanto os
 * índices físicos aparecem por baixo. Com frente = 5 e fim = 2 num vetor de 8,
 * a fila parece invertida na tela, e sem as duas leituras juntas não há como
 * entender o desenho.
 */
export function ordemLogica(
  capacidade: number,
  frente: number,
  quantidade: number,
): number[] {
  if (frente < 0 || quantidade <= 0 || capacidade <= 0) return [];
  const fisicos: number[] = [];
  for (let k = 0; k < Math.min(quantidade, capacidade); k++) {
    fisicos.push((frente + k) % capacidade);
  }
  return fisicos;
}
