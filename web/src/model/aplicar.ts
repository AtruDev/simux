/* web/src/model/aplicar.ts — evento -> mutação do modelo.
 *
 * É a peça mais fácil de errar em silêncio: um evento tratado ao contrário não
 * quebra nada, só faz a animação mentir. Daí os testes em
 * scripts/testar-modelo.ts. */

import type { Ev } from "../core/bridge";
import { EvKind, Tag } from "../core/ops";
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

    case EvKind.EV_ARR_INIT:
      m.vetor = {
        capacidade: ev.a,
        valores: new Array<number | null>(ev.a).fill(null),
        marcas: new Array<number>(ev.a).fill(Tag.TAG_NENHUMA),
        ultimoLido: -1,
        ultimoEscrito: -1,
        comparando: null,
        comparandoMao: false,
        faixa: null,
        aux: null,
        auxUltimoEscrito: -1,
      };
      break;

    case EvKind.EV_ARR_WRITE:
      if (m.vetor && ev.a >= 0 && ev.a < m.vetor.capacidade) {
        m.vetor.valores[ev.a] = ev.b;
        /* Escrever numa célula liberada a devolve ao uso. */
        m.vetor.marcas[ev.a] = Tag.TAG_NENHUMA;
        m.vetor.ultimoEscrito = ev.a;
        m.vetor.ultimoLido = -1;
        m.vetor.comparando = null;
      }
      break;

    case EvKind.EV_ARR_READ:
      if (m.vetor) {
        m.vetor.ultimoLido = ev.a;
        m.vetor.ultimoEscrito = -1;
      }
      break;

    case EvKind.EV_ARR_MARK:
      if (m.vetor && ev.a >= 0 && ev.a < m.vetor.capacidade) {
        m.vetor.marcas[ev.a] = ev.b;
        /* Marcar é o algoritmo mudando de assunto: a comparação anterior
         * acabou. Sem isto, as duas últimas células comparadas ficavam acesas
         * até o fim da animação, com o vetor já ordenado. */
        m.vetor.comparando = null;
      }
      break;

    case EvKind.EV_ARR_SWAP:
      if (m.vetor) {
        const i = ev.a;
        const j = ev.b;
        const t = m.vetor.valores[i] ?? null;
        m.vetor.valores[i] = m.vetor.valores[j] ?? null;
        m.vetor.valores[j] = t;
        m.vetor.ultimoEscrito = i;
        m.vetor.ultimoLido = -1;
        m.vetor.comparando = null;
      }
      break;

    /* A comparação é um instante, não um estado que dura: ela vale até a
     * próxima, ou até o algoritmo escrever alguma coisa. Quem faz o destaque
     * durar o suficiente para ser visto é o renderizador, com o brilho que
     * decai — o modelo continua sendo o que é verdade agora. */
    case EvKind.EV_ARR_COMPARE:
      if (m.vetor) {
        m.vetor.comparando = [ev.a, ev.b];
        /* c = 1 é o valor em mãos: b indexa o auxiliar, não o vetor. */
        m.vetor.comparandoMao = ev.c === 1;
        m.vetor.ultimoLido = -1;
        m.vetor.ultimoEscrito = -1;
      }
      break;

    case EvKind.EV_ARR_RANGE:
      if (m.vetor) m.vetor.faixa = [ev.a, ev.b];
      break;

    case EvKind.EV_AUX_INIT:
      if (m.vetor) {
        m.vetor.aux = new Array<number | null>(ev.a).fill(null);
        m.vetor.auxUltimoEscrito = -1;
      }
      break;

    case EvKind.EV_AUX_WRITE:
      if (m.vetor?.aux && ev.a >= 0 && ev.a < m.vetor.aux.length) {
        m.vetor.aux[ev.a] = ev.b;
        m.vetor.auxUltimoEscrito = ev.a;
      }
      break;

    case EvKind.EV_PHASE:
      m.fase = { str: ev.a, a: ev.b, b: ev.c };
      if (m.vetor) m.vetor.comparando = null;
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
      /* Os eventos de disco chegam na Fase 5. Ignorar o desconhecido é melhor
       * que quebrar: um trace com evento novo continua reproduzível no que
       * ele já entende. */
      break;
  }
}
