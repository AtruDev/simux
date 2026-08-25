/* web/src/model/aplicar.ts — evento -> mutação do modelo.
 *
 * É a peça mais fácil de errar em silêncio: um evento tratado ao contrário não
 * quebra nada, só faz a animação mentir. Daí os testes em
 * scripts/testar-modelo.ts. */

import type { Ev } from "../core/bridge";
import { EvKind } from "../core/ops";
import type { Modelo } from "./modelo";

function garantirNo(m: Modelo, id: number) {
  let no = m.nos.get(id);
  if (!no) {
    no = { id, valor: 0, arestas: new Map() };
    m.nos.set(id, no);
    m.ordem.push(id);
  }
  return no;
}

export function aplicar(m: Modelo, ev: Ev): void {
  /* Todo evento sabe de onde veio, e é isso que move o destaque no painel de
   * código. Eventos sem origem (src 0) não mexem no painel. */
  if (ev.src !== 0) {
    m.fonte = { src: ev.src, linha: ev.line };
  }

  switch (ev.kind) {
    case EvKind.EV_MSG:
      m.mensagem = ev.a;
      break;

    case EvKind.EV_COUNT:
      /* b é delta, não valor: o contador é acumulado pela reprodução. */
      m.contadores.set(ev.a, (m.contadores.get(ev.a) ?? 0) + ev.b);
      break;

    case EvKind.EV_NODE_NEW: {
      const no = garantirNo(m, ev.a);
      no.valor = ev.b;
      break;
    }

    case EvKind.EV_NODE_FREE:
      m.nos.delete(ev.a);
      m.ordem = m.ordem.filter((id) => id !== ev.a);
      m.visitados.delete(ev.a);
      /* Arestas que apontavam para ele viram NULL, senão o desenho ficaria
       * com seta para um nó que não existe mais. */
      for (const outro of m.nos.values()) {
        for (const [slot, destino] of outro.arestas) {
          if (destino === ev.a) outro.arestas.set(slot, 0);
        }
      }
      break;

    case EvKind.EV_NODE_SET:
      /* slot da chave em b; por ora só o slot 0 existe. */
      if (ev.b === 0) garantirNo(m, ev.a).valor = ev.c;
      break;

    case EvKind.EV_EDGE_SET:
      garantirNo(m, ev.a).arestas.set(ev.b, ev.c);
      break;

    case EvKind.EV_PTR_SET:
      m.ponteiros.set(ev.a, ev.b);
      break;

    case EvKind.EV_VISIT:
      m.visitados.add(ev.a);
      break;

    case EvKind.EV_UNVISIT:
      m.visitados.delete(ev.a);
      break;

    default:
      /* Os eventos de vetor e de disco chegam nas fases seguintes. Ignorar o
       * desconhecido é melhor que quebrar: um trace com evento novo continua
       * reproduzível no que ele já entende. */
      break;
  }
}
