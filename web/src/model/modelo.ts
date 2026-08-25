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
  };
}

/** Alvo de um ponteiro nomeado, ou 0 se ele aponta para NULL. */
export function alvoDe(m: Modelo, ptr: number): number {
  return m.ponteiros.get(ptr) ?? 0;
}

export function contador(m: Modelo, cnt: number): number {
  return m.contadores.get(cnt) ?? 0;
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
