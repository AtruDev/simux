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
  };
}

/** Alvo de um ponteiro nomeado, ou 0 se ele aponta para NULL. */
export function alvoDe(m: Modelo, ptr: number): number {
  return m.ponteiros.get(ptr) ?? 0;
}

export function contador(m: Modelo, cnt: number): number {
  return m.contadores.get(cnt) ?? 0;
}
