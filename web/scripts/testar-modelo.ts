/* Testes de aplicar.ts — evento para modelo.
 *
 * É a única parte do frontend que vale testar de verdade: um evento tratado ao
 * contrário não quebra nada, só faz a animação mentir. Um componente React
 * quebrado, ao contrário, aparece na hora.
 *
 * Sem framework, pelo mesmo motivo do runner em C: não é preciso. */

import type { Ev } from "../src/core/bridge";
import { Campo, Cnt, EvKind, Ptr, Src, Str, Tag } from "../src/core/ops";
import { Player } from "../src/core/player";
import { aplicar } from "../src/model/aplicar";
import {
  alturaDaArvore,
  alvoDe,
  contador,
  modeloNovo,
  type Modelo,
} from "../src/model/modelo";

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

CASO("modo comparar: as trilhas andam alinhadas por operação");
{
  /* Uma operação que a trilha 0 faz em três eventos e a trilha 1 em um. Os
   * quadros da operação têm que ser três: a trilha curta espera, não corre na
   * frente. Antes deles vem o quadro da sessão, que é onde o vetor nasce. */
  const player = new Player();
  player.carregarTrilhas([[[], [ev(EvKind.EV_ARR_INIT, 8)]]]);
  player.anexarTrilhas([
    [
      [
        ev(EvKind.EV_NODE_NEW, 1, 10),
        ev(EvKind.EV_EDGE_SET, 1, 0, 0),
        ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, 1),
      ],
      [ev(EvKind.EV_ARR_WRITE, 0, 10)],
    ],
  ]);

  igual(player.trilhas, 2, "duas trilhas");
  igual(player.ler().total, 4, "1 da sessão + 3 da trilha mais longa");

  /* No primeiro passo da operação as duas já agiram: a curta escreveu na
   * célula, a longa criou o nó. É o que faz a comparação ser legível. */
  player.irPara(2);
  igual(player.estadoDe(0).nos.size, 1, "trilha 0 criou o nó");
  igual(player.estadoDe(1).vetor?.valores[0], 10, "trilha 1 escreveu a célula");

  /* E no fim, a trilha curta não ganhou nada a mais por ter esperado. */
  player.irPara(4);
  igual(alvoDe(player.estadoDe(0), Ptr.PTR_TOPO), 1, "topo da trilha 0");
  igual(player.estadoDe(1).nos.size, 0, "a trilha 1 não tem nós");
  igual(player.estadoDe(1).vetor?.valores[0], 10, "e a célula continua escrita");
}

CASO("modo comparar: voltar reexecuta as duas trilhas");
{
  const player = new Player();
  player.carregarTrilhas([[[], [ev(EvKind.EV_ARR_INIT, 8)]]]);
  player.anexarTrilhas([
    [[ev(EvKind.EV_NODE_NEW, 1, 10)], [ev(EvKind.EV_ARR_WRITE, 0, 10)]],
    [[ev(EvKind.EV_NODE_NEW, 2, 20)], [ev(EvKind.EV_ARR_WRITE, 1, 20)]],
  ]);

  player.irPara(3);
  igual(player.estadoDe(0).nos.size, 2, "dois nós no fim");
  igual(player.estadoDe(1).vetor?.valores[1], 20, "e duas células escritas");

  /* Voltar um passo tem que desfazer nas DUAS. Reexecutar só a trilha 0 é o
   * erro que passaria despercebido: o canvas de cima ficaria certo e o de
   * baixo continuaria mostrando o futuro. */
  player.irPara(2);
  igual(player.estadoDe(0).nos.size, 1, "um nó ao voltar");
  igual(player.estadoDe(1).vetor?.valores[1] ?? null, null, "e a célula 1 vazia");
}

CASO("modo comparar: o histórico é por trilha");
{
  const player = new Player();
  player.carregarTrilhas([[[], []]]);
  player.anexarTrilhas([
    [
      [ev(EvKind.EV_NODE_NEW, 1, 10), ev(EvKind.EV_PTR_SET, Ptr.PTR_TOPO, 1)],
      [ev(EvKind.EV_COUNT, Cnt.CNT_ESCRITAS, +1)],
    ],
  ]);
  player.irPara(2);

  igual(player.historico(9, 0).length, 2, "dois eventos na trilha 0");
  /* Um só: o quadro em que a trilha 1 esperou não entra no log dela — esperar
   * não é uma coisa que ela fez. */
  igual(player.historico(9, 1).length, 1, "um evento na trilha 1");
}

/* ---- ordenação --------------------------------------------------------- */

CASO("a comparação vale até a próxima, e some quando o algoritmo escreve");
{
  const m = reproduzir([
    ev(EvKind.EV_ARR_INIT, 4),
    ev(EvKind.EV_ARR_COMPARE, 0, 1),
  ]);
  igual(m.vetor?.comparando?.[0] ?? -1, 0, "compara a célula 0");
  igual(m.vetor?.comparando?.[1] ?? -1, 1, "com a célula 1");
  ok(m.vetor?.comparandoMao === false, "e não é o valor em mãos");

  aplicar(m, ev(EvKind.EV_ARR_SWAP, 0, 1));
  ok(m.vetor?.comparando === null, "a troca apaga a comparação");
}

CASO("c = 1 é o valor em mãos, e não a célula b");
{
  const m = reproduzir([
    ev(EvKind.EV_ARR_INIT, 4),
    ev(EvKind.EV_AUX_INIT, 1),
    ev(EvKind.EV_AUX_WRITE, 0, 42),
    ev(EvKind.EV_ARR_COMPARE, 2, 0, 1),
  ]);
  ok(m.vetor?.comparandoMao === true, "a comparação é contra o auxiliar");
  igual(m.vetor?.aux?.[0] ?? null, 42, "que guarda o valor em mãos");
  /* Se o renderizador tratasse b como índice do vetor, acenderia a célula 0
   * enquanto a comparação é com a 2. Foi por isto que o c = 1 existe. */
  igual(m.vetor?.comparando?.[0] ?? -1, 2, "a célula comparada é a 2");
}

CASO("a faixa ativa e o auxiliar do merge");
{
  const m = reproduzir([
    ev(EvKind.EV_ARR_INIT, 8),
    ev(EvKind.EV_ARR_RANGE, 2, 5),
    ev(EvKind.EV_AUX_INIT, 8),
    ev(EvKind.EV_AUX_WRITE, 2, 7),
    ev(EvKind.EV_AUX_WRITE, 3, 9),
  ]);
  igual(m.vetor?.faixa?.[0] ?? -1, 2, "faixa começa em 2");
  igual(m.vetor?.faixa?.[1] ?? -1, 5, "e termina em 5");
  igual(m.vetor?.aux?.length ?? 0, 8, "auxiliar do tamanho do vetor");
  igual(m.vetor?.aux?.[3] ?? null, 9, "com o valor escrito");
  igual(m.vetor?.auxUltimoEscrito ?? -1, 3, "e a última escrita marcada");
}

CASO("a fase carrega os operandos, não a frase");
{
  const m = reproduzir([
    ev(EvKind.EV_ARR_INIT, 8),
    ev(EvKind.EV_PHASE, Str.STR_INTERCALANDO, 0, 3),
  ]);
  igual(m.fase?.str ?? -1, Str.STR_INTERCALANDO, "o id da mensagem");
  igual(m.fase?.a ?? -1, 0, "e os extremos do trecho");
  igual(m.fase?.b ?? -1, 3, "que vieram em b e c");
}

CASO("os contadores da ordenação vêm dos eventos, como os das estruturas");
{
  const m = reproduzir([
    ev(EvKind.EV_ARR_INIT, 4),
    ev(EvKind.EV_ARR_COMPARE, 0, 1),
    ev(EvKind.EV_COUNT, Cnt.CNT_COMPARACOES, +1),
    ev(EvKind.EV_ARR_SWAP, 0, 1),
    ev(EvKind.EV_COUNT, Cnt.CNT_ESCRITAS, +2),
  ]);
  igual(contador(m, Cnt.CNT_COMPARACOES), 1, "uma comparação");
  igual(contador(m, Cnt.CNT_ESCRITAS), 2, "duas escritas na troca");
}

CASO("voltar no tempo desfaz a ordenação inteira, reexecutando");
{
  const player = new Player();
  const cena = [
    ev(EvKind.EV_ARR_INIT, 3),
    ev(EvKind.EV_ARR_WRITE, 0, 3),
    ev(EvKind.EV_ARR_WRITE, 1, 1),
    ev(EvKind.EV_ARR_WRITE, 2, 2),
  ];
  const ordenar = [
    ev(EvKind.EV_ARR_COMPARE, 0, 1),
    ev(EvKind.EV_ARR_SWAP, 0, 1),
    ev(EvKind.EV_ARR_MARK, 2, Tag.TAG_ORDENADO),
  ];
  player.carregarTrilhas([[cena], [ordenar]]);

  igual(player.estadoDe(0).vetor?.valores[0] ?? null, 1, "ordenado no fim");

  /* Ir ao passo em que a cena acabou tem que devolver o vetor original — e
   * chegar lá é reexecutar do zero, não desfazer a troca. */
  player.irPara(cena.length);
  igual(player.estadoDe(0).vetor?.valores[0] ?? null, 3, "e o original ao voltar");
  igual(player.estadoDe(0).vetor?.valores[1] ?? null, 1, "com a célula 1 intacta");
  igual(
    player.estadoDe(0).vetor?.marcas[2] ?? -1,
    Tag.TAG_NENHUMA,
    "e sem a marca de ordenado",
  );
}

/* ---- árvore ------------------------------------------------------------ */

/* Os eventos que o abb.c emite ao criar um nó, na ordem. Os dois filhos nulos
 * são anunciados de propósito: o desenho precisa saber que o nó tem dois
 * lugares vazios, e não que ele não tem lugar nenhum. */
function noArvore(id: number, valor: number): Ev[] {
  return [
    ev(EvKind.EV_NODE_NEW, id, valor),
    ev(EvKind.EV_EDGE_SET, id, 0, 0),
    ev(EvKind.EV_EDGE_SET, id, 1, 0),
  ];
}

CASO("a altura é medida no desenho, não recebida do C");
{
  const m = reproduzir([
    ...noArvore(1, 50),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    ...noArvore(2, 30),
    ev(EvKind.EV_EDGE_SET, 1, 0, 2),
    ...noArvore(3, 70),
    ev(EvKind.EV_EDGE_SET, 1, 1, 3),
    ...noArvore(4, 20),
    ev(EvKind.EV_EDGE_SET, 2, 0, 4),
  ]);

  igual(alturaDaArvore(m, alvoDe(m, Ptr.PTR_RAIZ)), 3, "três níveis");
  igual(m.nos.size, 4, "quatro nós");
}

CASO("a árvore degenerada tem altura n, e é isso que a AVL vai desentortar");
{
  const eventos: Ev[] = [
    ...noArvore(1, 1),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
  ];
  for (let k = 2; k <= 8; k++) {
    eventos.push(...noArvore(k, k), ev(EvKind.EV_EDGE_SET, k - 1, 1, k));
  }
  const m = reproduzir(eventos);

  igual(
    alturaDaArvore(m, alvoDe(m, Ptr.PTR_RAIZ)),
    8,
    "oito níveis para oito nós",
  );
}

CASO("a altura de uma árvore vazia é zero, e um ciclo não trava a medida");
{
  const m = reproduzir([]);
  igual(alturaDaArvore(m, 0), 0, "sem raiz");

  /* Um EV_EDGE_SET defeituoso pode fechar um ciclo. Uma árvore não tem
   * ciclos, mas travar a aba num laço infinito seria o pior jeito de
   * descobrir isso. */
  const ciclo = reproduzir([
    ...noArvore(1, 1),
    ...noArvore(2, 2),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    ev(EvKind.EV_EDGE_SET, 1, 1, 2),
    ev(EvKind.EV_EDGE_SET, 2, 1, 1),
  ]);
  igual(alturaDaArvore(ciclo, 1), 2, "para no nó já visto");
}

CASO("EV_NODE_SET troca o valor no lugar — é o sucessor subindo");
{
  const m = reproduzir([
    ...noArvore(1, 50),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    /* slot 0 é o da chave; o C manda o valor em c. */
    ev(EvKind.EV_NODE_SET, 1, 0, 60),
  ]);
  igual(m.nos.get(1)?.valor ?? -1, 60, "o nó ficou com o valor do sucessor");
  igual(m.nos.size, 1, "e nenhum nó foi criado por isso");
}

CASO("liberar um nó desfaz as arestas que apontavam para ele");
{
  const m = reproduzir([
    ...noArvore(1, 50),
    ...noArvore(2, 30),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    ev(EvKind.EV_EDGE_SET, 1, 0, 2),
    ev(EvKind.EV_NODE_FREE, 2),
  ]);
  igual(m.nos.size, 1, "sobrou a raiz");
  igual(m.nos.get(1)?.arestas.get(0) ?? -1, 0, "e o filho esquerdo virou NULL");
  igual(alturaDaArvore(m, 1), 1, "com a altura caindo junto");
}

CASO("o FB entra por EV_NODE_SET, no campo dele, e não vira valor");
{
  const m = reproduzir([
    ...noArvore(1, 50),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    ev(EvKind.EV_NODE_SET, 1, Campo.CAMPO_FB, 2),
  ]);
  igual(m.nos.get(1)?.fb ?? null, 2, "o FB chegou");
  igual(m.nos.get(1)?.valor ?? -1, 50, "e a chave não foi tocada");
}

CASO("quem não é AVL não tem FB, e desenhar zero seria mentira");
{
  const m = reproduzir([...noArvore(1, 50)]);
  igual(m.nos.get(1)?.fb ?? "sem", "sem", "nulo, e não zero");
}

CASO("a rotação religa as arestas, e a altura cai junto");
{
  /* 30, 20, 10 na AVL: caso esquerda-esquerda. Depois da rotação à direita,
   * o 20 é a raiz e os outros dois são folhas. */
  const m = reproduzir([
    ...noArvore(1, 30),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 1),
    ...noArvore(2, 20),
    ev(EvKind.EV_EDGE_SET, 1, 0, 2),
    ...noArvore(3, 10),
    ev(EvKind.EV_EDGE_SET, 2, 0, 3),
    /* a rotação: 20 sobe, 30 vira filho direito dele */
    ev(EvKind.EV_EDGE_SET, 1, 0, 0),
    ev(EvKind.EV_EDGE_SET, 2, 1, 1),
    ev(EvKind.EV_PTR_SET, Ptr.PTR_RAIZ, 2),
    ev(EvKind.EV_NODE_SET, 1, Campo.CAMPO_FB, 0),
    ev(EvKind.EV_NODE_SET, 2, Campo.CAMPO_FB, 0),
  ]);

  igual(alvoDe(m, Ptr.PTR_RAIZ), 2, "o 20 virou raiz");
  igual(alturaDaArvore(m, 2), 2, "e a altura caiu de três para dois");
  igual(m.nos.get(2)?.arestas.get(0) ?? -1, 3, "10 à esquerda");
  igual(m.nos.get(2)?.arestas.get(1) ?? -1, 1, "30 à direita");
  igual(m.nos.get(1)?.arestas.get(0) ?? -1, 0, "e o 30 soltou o filho");
}

if (falhas.length > 0) {
  console.error("testar-modelo falhou:");
  for (const f of falhas) console.error(`  - ${f}`);
  throw new Error(`testar-modelo: ${falhas.length} falha(s)`);
}

console.log(`testar-modelo: ok (${checagens} checagens)`);
