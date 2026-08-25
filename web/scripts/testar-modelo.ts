/* Testes de aplicar.ts — evento para modelo.
 *
 * É a única parte do frontend que vale testar de verdade: um evento tratado ao
 * contrário não quebra nada, só faz a animação mentir. Um componente React
 * quebrado, ao contrário, aparece na hora.
 *
 * Sem framework, pelo mesmo motivo do runner em C: não é preciso. */

import type { Ev } from "../src/core/bridge";
import { Cnt, EvKind, Ptr, Src } from "../src/core/ops";
import { aplicar } from "../src/model/aplicar";
import { alvoDe, contador, modeloNovo, type Modelo } from "../src/model/modelo";

let checagens = 0;
const falhas: string[] = [];
let caso = "(sem nome)";

function CASO(nome: string) {
  caso = nome;
}

function ok(condicao: boolean, oQue: string) {
  checagens++;
  if (!condicao) falhas.push(`${caso}: ${oQue}`);
}

function igual(obtido: unknown, esperado: unknown, oQue: string) {
  checagens++;
  if (obtido !== esperado) {
    falhas.push(`${caso}: ${oQue} — obtido ${String(obtido)}, esperado ${String(esperado)}`);
  }
}

function ev(kind: number, a = 0, b = 0, c = 0, line = 1): Ev {
  return { kind, src: Src.SRC_PILHA_ENC, line, a, b, c };
}

/** Reproduz uma lista de eventos num modelo novo, como o Player faz. */
function reproduzir(eventos: Ev[]): Modelo {
  const m = modeloNovo();
  for (const e of eventos) aplicar(m, e);
  return m;
}

/* Os eventos que o pilha_enc.c emite num push, na ordem. */
function push(id: number, valor: number, anterior: number): Ev[] {
  return [
    ev(EvKind.EV_NODE_NEW, id, valor),
    ev(EvKind.EV_EDGE_SET, id, 0, anterior),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, id),
    ev(EvKind.EV_COUNT, Cnt.CNT_TAMANHO, +1),
    ev(EvKind.EV_COUNT, Cnt.CNT_ALOCACOES, +1),
  ];
}

/* ------------------------------------------------------------------------ */

CASO("push constrói a cadeia e move o topo");
{
  const m = reproduzir([...push(1, 10, 0), ...push(2, 20, 1)]);
  igual(m.nos.size, 2, "dois nós");
  igual(m.nos.get(2)?.valor, 20, "valor do topo");
  igual(m.nos.get(2)?.arestas.get(0), 1, "o novo aponta para o anterior");
  igual(m.nos.get(1)?.arestas.get(0), 0, "o primeiro aponta para NULL");
  igual(alvoDe(m, Ptr.PTR_TOPO), 2, "topo é o último empilhado");
  igual(contador(m, Cnt.CNT_TAMANHO), 2, "tamanho acumulado");
}

CASO("EV_COUNT acumula delta, não substitui valor");
{
  /* Se alguém tratar b como valor absoluto em vez de delta, o contador ainda
   * parece certo em muitos casos — e erra exatamente no pop. */
  const m = reproduzir([
    ...push(1, 10, 0),
    ...push(2, 20, 1),
    ev(EvKind.EV_COUNT, Cnt.CNT_TAMANHO, -1),
  ]);
  igual(contador(m, Cnt.CNT_TAMANHO), 1, "2 - 1");
  igual(contador(m, Cnt.CNT_ALOCACOES), 2, "alocações não diminuem no pop");
}

CASO("pop remove o nó e nenhuma seta sobra apontando para ele");
{
  const m = reproduzir([
    ...push(1, 10, 0),
    ...push(2, 20, 1),
    ev(EvKind.EV_VISIT, 2),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, 1),
    ev(EvKind.EV_NODE_FREE, 2),
    ev(EvKind.EV_COUNT, Cnt.CNT_TAMANHO, -1),
  ]);
  igual(m.nos.size, 1, "sobrou um nó");
  ok(!m.nos.has(2), "o nó liberado saiu do modelo");
  ok(!m.visitados.has(2), "e saiu dos visitados junto");
  igual(alvoDe(m, Ptr.PTR_TOPO), 1, "topo voltou ao anterior");
}

CASO("nenhuma aresta aponta para um nó que não existe mais");
{
  /* Este é o caso que produziria seta para o vazio na tela: se o trace
   * liberasse um nó sem religar quem apontava para ele. */
  const m = reproduzir([
    ...push(1, 10, 0),
    ...push(2, 20, 1),
    ev(EvKind.EV_NODE_FREE, 1),
  ]);
  igual(m.nos.get(2)?.arestas.get(0), 0, "a aresta virou NULL");
}

CASO("reproduzir um prefixo dá o mesmo estado de nunca ter ido além");
{
  /* É a premissa de voltar no tempo: ir ao passo k é zerar e aplicar 0..k.
   * Se isto falhar, arrastar a barra para trás mostra estado errado. */
  const completo = [...push(1, 10, 0), ...push(2, 20, 1), ...push(3, 30, 2)];
  const prefixo = completo.slice(0, 10);

  const direto = reproduzir(prefixo);
  const voltando = reproduzir(completo);
  const reconstruido = reproduzir(prefixo);

  igual(direto.nos.size, reconstruido.nos.size, "mesmo número de nós");
  igual(
    alvoDe(direto, Ptr.PTR_TOPO),
    alvoDe(reconstruido, Ptr.PTR_TOPO),
    "mesmo topo",
  );
  igual(voltando.nos.size, 3, "o completo tem três nós");
  igual(direto.nos.size, 2, "o prefixo de dez eventos tem dois");
}

CASO("todo evento com origem move o destaque do código");
{
  const m = reproduzir([ev(EvKind.EV_NODE_NEW, 1, 10, 0, 48)]);
  igual(m.fonte?.src, Src.SRC_PILHA_ENC, "origem");
  igual(m.fonte?.linha, 48, "linha");
}

/* ------------------------------------------------------------------------ */

if (falhas.length > 0) {
  console.error("testar-modelo falhou:");
  for (const f of falhas) console.error(`  - ${f}`);
  throw new Error(`testar-modelo: ${falhas.length} falha(s)`);
}

console.log(`testar-modelo: ok (${checagens} checagens)`);
