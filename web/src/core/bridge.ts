/* web/src/core/bridge.ts — a única porta para o wasm.
 *
 * O C é a fonte da verdade lógica; o JS mantém o próprio modelo aplicando os
 * eventos. Nada de estado da estrutura atravessa a fronteira: só o trace. */

import criarSimux, { type ModuloSimux } from "../wasm/simux.js";
import { EV_CAMPOS, type Op } from "./ops";

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

/**
 * Executa uma operação e devolve os eventos que ela emitiu.
 *
 * O trace é zerado a cada chamada, do lado do C.
 */
export function exec(op: Op, a = 0, b = 0, c = 0): Ev[] {
  const m = modulo;
  if (!m) {
    throw new Error("bridge: chame iniciar() antes de exec()");
  }

  const rc = m._ds_call(op, a, b, c);
  if (rc < 0) {
    throw new ErroDs(m._ds_erro());
  }

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
