/* web/src/core/bridge.ts — a única porta para o wasm.
 *
 * O C é a fonte da verdade lógica; o JS mantém o próprio modelo aplicando os
 * eventos. Nada de estado da estrutura atravessa a fronteira: só o trace. */

import criarSimux, { type ModuloSimux } from "../wasm/simux.js";
import { EV_CAMPOS, Status, type Op, type Tipo } from "./ops";

/** Um evento do trace, já em forma de objeto. */
export interface Ev {
  kind: number;
  src: number;
  line: number;
  a: number;
  b: number;
  c: number;
}

/** Falha vinda do core. O código é um ERR_* de erros.h; a frase vem do i18n. */
export class ErroDs extends Error {
  constructor(readonly codigo: number) {
    super(`ds_call devolveu o erro ${codigo}`);
    this.name = "ErroDs";
  }
}

let modulo: ModuloSimux | null = null;

/** Carrega o módulo. Idempotente. */
export async function iniciar(): Promise<void> {
  modulo ??= await criarSimux();
}

export function pronto(): boolean {
  return modulo !== null;
}

/** Abre uma sessão sobre uma estrutura, descartando a anterior.
 *
 * A capacidade é ignorada pelas implementações encadeadas, que não têm
 * limite — mas passá-la sempre mantém uma chamada só. */
export function sessaoNova(tipo: Tipo, capacidade = 8): Ev[] {
  const m = exigirModulo();
  if (m._ds_sessao_nova(tipo, capacidade) < 0) {
    throw new ErroDs(m._ds_erro());
  }
  return lerTrace(m);
}

/** Capacidade da sessão, ou -1 quando não há limite. */
export function capacidade(): number {
  return modulo ? modulo._ds_capacidade() : -1;
}

export function sessaoFim(): void {
  modulo?._ds_sessao_fim();
}

/** O que uma operação produziu: os eventos, e como ela terminou. */
export interface Saida {
  eventos: Ev[];
  /** OK, ou o ERR_* de erros.h que a operação devolveu. */
  erro: number;
}

/**
 * Executa uma operação e devolve os eventos que ela emitiu.
 *
 * O trace é zerado a cada chamada, do lado do C.
 *
 * O erro vem ao lado dos eventos em vez de virar exceção porque uma operação
 * recusada também é uma coisa para assistir: desempilhar uma pilha vazia emite
 * o EV_MSG que explica a recusa, e carrega a linha de pilha_enc.c que a
 * detectou. Lançar aqui jogava esse trace fora, e o painel de código ficava
 * mostrando a operação anterior enquanto a mensagem de erro aparecia.
 */
export function chamar(op: Op, a = 0, b = 0, c = 0): Saida {
  const m = exigirModulo();

  const rc = m._ds_call(op, a, b, c);
  return { eventos: lerTrace(m), erro: rc < 0 ? m._ds_erro() : Status.OK };
}

function exigirModulo(): ModuloSimux {
  if (!modulo) {
    throw new Error("bridge: chame iniciar() antes de usar o core");
  }
  return modulo;
}

function lerTrace(m: ModuloSimux): Ev[] {
  const ptr = m._ds_trace_ptr();
  const total = m._ds_trace_len();

  /* A view é criada aqui, a cada chamada, e nunca guardada: com
   * ALLOW_MEMORY_GROWTH qualquer malloc que cresça a heap desanexa as views
   * antigas. Guardar uma em variável de módulo é bug garantido — e só
   * aparece quando o dataset cresce, que é o pior momento para descobrir. */
  const bruto = new Int32Array(m.HEAP32.buffer, ptr, total * EV_CAMPOS);

  const eventos: Ev[] = new Array<Ev>(total);
  for (let i = 0; i < total; i++) {
    const o = i * EV_CAMPOS;
    eventos[i] = {
      kind: bruto[o] ?? 0,
      src: bruto[o + 1] ?? 0,
      line: bruto[o + 2] ?? 0,
      a: bruto[o + 3] ?? 0,
      b: bruto[o + 4] ?? 0,
      c: bruto[o + 5] ?? 0,
    };
  }
  return eventos;
}

/** Verdadeiro se a última operação encheu o buffer e perdeu eventos. */
export function truncado(): boolean {
  return modulo !== null && modulo._ds_trace_truncado() !== 0;
}
