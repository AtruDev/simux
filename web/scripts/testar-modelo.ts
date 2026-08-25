/* Testes de aplicar.ts — evento para modelo.
 *
 * É a única parte do frontend que vale testar de verdade: um evento tratado ao
 * contrário não quebra nada, só faz a animação mentir. Um componente React
 * quebrado, ao contrário, aparece na hora.
 *
 * Sem framework, pelo mesmo motivo do runner em C: não é preciso. */

import type { Ev } from "../src/core/bridge";
import { Cnt, EvKind, Ptr, Src } from "../src/core/ops";
import { Player } from "../src/core/player";
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


/* ---- Player ------------------------------------------------------------- *
 * O Player não precisa de navegador enquanto o laço não é iniciado: só
 * iniciarLaco() toca em requestAnimationFrame. Dá para testar a linha do
 * tempo inteira aqui, e é onde mora o critério de pronto da fase.           */

CASO("operar não descarta o que já aconteceu");
{
  /* Foi o bug: anexar() truncava a linha do tempo no cursor, para descartar
   * "o futuro" de um scrub. Mas operar antes de a animação anterior terminar
   * deixa o cursor atrás sem que nada tenha sido desfeito — e os pushes
   * anteriores sumiam. A estrutura que o C tem reflete todas as operações,
   * esteja o cursor onde estiver. */
  const player = new Player();
  player.carregar([ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, 0)]);

  player.anexar(push(1, 10, 0));
  player.anexar(push(2, 20, 1)); /* cursor ainda lá atrás, animação rodando */
  player.anexar(push(3, 30, 2));

  igual(player.ler().total, 16, "1 da sessão + 3 x 5 do push");

  player.irPara(player.ler().total);
  igual(player.estado.nos.size, 3, "os três nós existem");
  igual(contador(player.estado, Cnt.CNT_TAMANHO), 3, "tamanho 3");
  igual(alvoDe(player.estado, Ptr.PTR_TOPO), 3, "topo é o último");
}

CASO("anexar volta ao instante anterior aos eventos novos");
{
  const player = new Player();
  player.carregar([]);
  player.anexar(push(1, 10, 0));
  igual(player.ler().i, 0, "cursor no começo dos eventos novos");
  player.anexar(push(2, 20, 1));
  igual(player.ler().i, 5, "cursor logo antes do segundo push");
  igual(player.estado.nos.size, 1, "e o estado é o de antes dele");
}

CASO("ir e voltar dá o mesmo estado — o critério da fase");
{
  const player = new Player();
  player.carregar([
    ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, 0),
    ...push(1, 10, 0),
    ...push(2, 20, 1),
    ...push(3, 30, 2),
    ...push(4, 40, 3),
    ...push(5, 50, 4),
  ]);
  const total = player.ler().total;

  player.irPara(total);
  const noFim = {
    nos: player.estado.nos.size,
    topo: alvoDe(player.estado, Ptr.PTR_TOPO),
    tamanho: contador(player.estado, Cnt.CNT_TAMANHO),
  };
  igual(noFim.nos, 5, "cinco nós no fim");
  igual(noFim.tamanho, 5, "tamanho 5 no fim");

  /* volta passo a passo até o começo */
  for (let k = total; k > 0; k--) player.irPara(k - 1);
  igual(player.ler().i, 0, "voltou ao início");
  igual(player.estado.nos.size, 0, "nenhum nó no início");
  igual(contador(player.estado, Cnt.CNT_TAMANHO), 0, "tamanho zerado");

  /* e avança de novo até o fim */
  for (let k = 0; k < total; k++) player.irPara(k + 1);
  igual(player.estado.nos.size, noFim.nos, "mesmos nós na volta");
  igual(alvoDe(player.estado, Ptr.PTR_TOPO), noFim.topo, "mesmo topo");
  igual(
    contador(player.estado, Cnt.CNT_TAMANHO),
    noFim.tamanho,
    "mesmo tamanho",
  );

  /* e um salto direto para o meio bate com a reprodução de um prefixo */
  player.irPara(8);
  const pulando = player.estado.nos.size;
  player.irPara(0);
  player.irPara(8);
  igual(player.estado.nos.size, pulando, "saltar bate com avançar");
}

CASO("passo não passa dos limites");
{
  const player = new Player();
  player.carregar(push(1, 10, 0));
  player.passo(-5);
  igual(player.ler().i, 0, "não vai abaixo de zero");
  player.passo(99);
  igual(player.ler().i, 5, "não passa do total");
}


CASO("trocar de linha do tempo avisa mesmo com o mesmo tamanho");
{
  /* Duas sessões diferentes podem emitir o mesmo número de eventos — é o que
   * acontece ao trocar só a capacidade. Sem a geração, i e total ficavam
   * iguais, a foto era considerada inalterada, e os painéis continuavam
   * mostrando os números da estrutura anterior enquanto o canvas já mostrava
   * a nova. */
  const player = new Player();
  player.carregar([ev(EvKind.EV_ARR_INIT, 8)]);
  const antes = player.ler();

  player.carregar([ev(EvKind.EV_ARR_INIT, 4)]);
  const depois = player.ler();

  igual(antes.i, depois.i, "mesmo indice");
  igual(antes.total, depois.total, "mesmo total");
  ok(antes.geracao !== depois.geracao, "mas a geracao muda");
  igual(player.estado.vetor?.capacidade, 4, "e o modelo é o novo");
}

/* ------------------------------------------------------------------------ */

if (falhas.length > 0) {
  console.error("testar-modelo falhou:");
  for (const f of falhas) console.error(`  - ${f}`);
  throw new Error(`testar-modelo: ${falhas.length} falha(s)`);
}

console.log(`testar-modelo: ok (${checagens} checagens)`);
