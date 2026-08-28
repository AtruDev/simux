import type { Dicionario } from "./index";

const en: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "A visual simulator for data structures. The core is C compiled to " +
    "WebAssembly; the page animates what it executed.",
  "app.trocarIdioma": "Português",
  "app.copiarLink": "copy link",
  "app.linkCopiado": "copied",
  "app.carregando": "loading the core…",
  "app.erro": "error",
  "app.truncado": "trace truncated — events were dropped",

  "painel.operacoes": "Operations",
  "painel.codigo": "Source",
  "painel.metricas": "Metrics",
  "painel.log": "Log",

  "estrutura.titulo": "Structure",
  "estrutura.pilhaEnc": "Linked stack",

  "op.valor": "value",
  "op.push": "push",
  "op.pop": "pop",
  "op.topo": "peek",
  "op.limpar": "clear",
  "op.aleatorio": "random",

  "transporte.inicio": "back to start",
  "transporte.anterior": "previous step",
  "transporte.tocar": "play",
  "transporte.pausar": "pause",
  "transporte.proximo": "next step",
  "transporte.fim": "jump to end",
  "transporte.velocidade": "speed",
  "transporte.passo": "step",

  "metrica.tamanho": "size",
  "metrica.alocacoes": "allocations",
  "metrica.nos": "nodes on screen",
  "metrica.eventos": "events",

  "log.vazio": "Nothing has happened yet.",
  "log.noCriado": "node created",
  "log.noLiberado": "node freed",
  "log.aresta": "points to",
  "log.ponteiro": "top becomes",
  "log.visita": "looking at node",
  "log.saiVisita": "releases node",
  "log.contador": "counter",
  "estrutura.pilhaVet": "Array stack",
  "estrutura.filaEnc": "Linked queue",
  "estrutura.filaVet": "Circular queue",
  "estrutura.encadeada": "linked",
  "estrutura.vetor": "array",
  "op.enfileirar": "enqueue",
  "op.desenfileirar": "dequeue",
  "op.frente": "peek front",
  "op.capacidade": "capacity",
  "metrica.escritas": "writes",
  "metrica.capacidade": "capacity",
  "metrica.ocupacao": "occupancy",
  "log.escreve": "writes cell",
  "log.le": "reads cell",
  "log.marcaLivre": "cell released",
  "log.vetorInicia": "array of",
  "log.frente": "front becomes",
  "log.fim": "rear becomes",
  "log.nulo": "NULL",

  "estrutura.listaSimples": "Singly linked list",
  "estrutura.listaDupla": "Doubly linked list",
  "estrutura.listaCircular": "Circular list",

  "op.inserirInicio": "insert at head",
  "op.inserirFim": "insert at tail",
  "op.inserirEm": "insert at position",
  "op.removerEm": "remove at position",
  "op.removerInicio": "remove from head",
  "op.buscar": "search",
  "op.posicao": "position",
  "op.primeiro": "peek first",

  "metrica.comparacoes": "comparisons",

  "log.cursor": "cursor becomes",
  "log.inicio": "head becomes",

  "estrutura.comparar": "compare both implementations",

  "painel.script": "Operation script",
  "script.exemplo": "push 10\npush 20\npop\n# or: i 1 2 3",
  "script.ajuda":
    "One operation per line, or comma-separated. Ctrl+Enter runs it.",
  "script.rodar": "run script",
  "script.linha": "line",

  STR_NENHUMA: "",
  STR_PING: "the core answered",
  STR_PILHA_VAZIA: "the stack is empty",
  STR_PILHA_CHEIA: "the stack is full — n == capacity",
  STR_FILA_VAZIA: "the queue is empty",
  STR_FILA_CHEIA: "the queue is full — n == capacity",
  STR_DEU_VOLTA: "the index wrapped around",
  STR_LISTA_VAZIA: "the list is empty",
  STR_POSICAO_INVALIDA: "position outside the list",
  STR_ACHOU: "found",
  STR_NAO_ACHOU: "not in the list",
  STR_ANDANDO: "walking to the position",

  "aba.estruturas": "Structures",
  "aba.ordenacao": "Sorting",

  "painel.algoritmo": "Algorithm",
  "painel.cena": "Initial array",
  "painel.empirico": "Empirical mode",
  "painel.legenda": "Legend",
  "painel.fase": "Phase",

  "alg.bolha": "Bubble",
  "alg.selecao": "Selection",
  "alg.insercao": "Insertion",
  "alg.shell": "Shellsort",
  "alg.quick": "Quicksort",
  "alg.merge": "Mergesort",
  "alg.externa": "External merge sort",
  "ordem.externa": "1 + ⌈log₂(n/k)⌉ passes",
  "ord.memoria": "memory k",
  "ord.memoriaPorque":
    "How many records fit in RAM. The rest of the array lives \"on disk\": " +
    "every pass reads and writes the whole file, and doubling k removes one pass.",
  "metrica.passadas": "passes",

  "ordem.quadratica": "O(n²)",
  "ordem.linearitmica": "O(n log n)",
  "ordem.shell": "between O(n log² n) and O(n²)",

  "dist.aleatorio": "random",
  "dist.aleatorioPorque": "the average case, which is what the tables measure",
  "dist.quaseOrdenado": "nearly sorted",
  "dist.quaseOrdenadoPorque":
    "insertion beats quicksort here, and bubble exits after one pass",
  "dist.inverso": "reverse sorted",
  "dist.inversoPorque":
    "insertion sort's worst case, and bubble sort's maximum swaps",
  "dist.poucosDistintos": "few distinct values",
  "dist.poucosDistintosPorque":
    "breaks naive-pivot quicksort: nearly everything lands on one side",
  "dist.ordenado": "already sorted",
  "dist.ordenadoPorque":
    "bubble stops after one pass; selection costs exactly the same as always",
  "dist.manual": "manual",
  "dist.manualPorque": "the values from the exercise, pasted as they are",

  "ord.tamanho": "size",
  "ord.semente": "seed",
  "ord.distribuicao": "distribution",
  "ord.gerar": "generate array",
  "ord.novaSemente": "roll a seed",
  "ord.ordenar": "sort",
  "ord.corrida": "race mode",
  "ord.valores": "values",
  "ord.valoresAjuda": "Separate with commas, spaces or line breaks.",
  "ord.semFase": "idle",

  "legenda.comparando": "comparing",
  "legenda.escrita": "just written",
  "legenda.ordenado": "in final position",
  "legenda.pivo": "pivot",
  "legenda.auxiliar": "auxiliary memory (dashed)",
  "legenda.fora": "outside the active range",
  "legenda.cursores": "cursors i, j and min",

  "empirico.rodar": "measure",
  "empirico.rodando": "measuring…",
  "empirico.metrica": "metric",
  "empirico.comparacoes": "comparisons",
  "empirico.escritas": "writes",
  "empirico.vazio": "Nothing measured yet.",
  "empirico.explica":
    "Every point is a real run, with the trace switched off. The thin lines " +
    "are the theoretical curves, anchored at each algorithm's first point — " +
    "if the measurement follows the curve, the theory holds.",
  "empirico.teoria": "theoretical curves",
  "empirico.limite": "each algorithm runs as far as it is still worth measuring",

  "log.compara": "compare",
  "log.emMaos": "against the held value",
  "log.troca": "swap",
  "log.faixa": "active range",
  "log.auxInicia": "auxiliary of",
  "log.auxEscreve": "write to auxiliary",
  "log.fase": "phase",
  "log.marcaOrdenado": "in place",
  "log.marcaPivo": "becomes the pivot",

  STR_ORDENADO: "array sorted",
  STR_SEM_TROCAS: "no swaps in this pass — it is already sorted",
  STR_PASSADA: "pass",
  STR_PROCURANDO_MIN: "looking for the smallest in the range",
  STR_DESLOCANDO: "shifting right to open a slot",
  STR_GAP: "pass with gap",
  STR_PARTICIONANDO: "partitioning",
  STR_DIVIDINDO: "splitting",
  STR_INTERCALANDO: "merging",

  "estrutura.buscaSeq": "Sequential search",
  "estrutura.buscaBin": "Binary search",
  "op.inserirOrdenado": "insert in order",
  "op.removerMenor": "remove the smallest",
  "op.menor": "peek at the smallest",
  "legenda.chave": "key being searched for",
  "legenda.faixaViva": "range that can still hold the key",
  "legenda.achado": "found",

  STR_VETOR_CHEIO: "the array is full — n == capacity",
  STR_DESCARTA_ESQ: "the key is larger: the bottom half goes",
  STR_DESCARTA_DIR: "the key is smaller: the top half goes",

  "estrutura.abb": "Binary search tree",
  "op.inserir": "insert",
  "op.removerValor": "remove the value",
  "op.percurso": "traverse",
  "perc.emOrdem": "in-order",
  "perc.preOrdem": "pre-order",
  "perc.posOrdem": "post-order",
  "metrica.altura": "height",
  "metrica.alturaIdeal": "minimum height",

  STR_VAI_ESQ: "the key is smaller: go left",
  STR_VAI_DIR: "the key is larger: go right",
  STR_JA_EXISTE: "the value is already in the tree",
  STR_CASO_FOLHA: "case 1: the node is a leaf — nothing replaces it",
  STR_CASO_UM_FILHO: "case 2: the only child moves up into its place",
  STR_CASO_DOIS_FILHOS:
    "case 3: two children — the in-order successor moves up",
  STR_PROCURA_SUCESSOR:
    "the successor is the smallest of the right subtree: right once, then left all the way",
  STR_SUBSTITUI: "the successor's value takes the place, and the successor is what goes",
  STR_PERCURSO: "traversing",

  "log.raiz": "root is now",
  "log.esquerda": "left →",
  "log.direita": "right →",

  "estrutura.avl": "AVL tree",
  "metrica.rotacoes": "rotations",

  STR_DESBALANCEOU: "the balance factor overflowed: |BF| > 1",
  STR_ROT_DIR: "left-left case: a single right rotation fixes it",
  STR_ROT_ESQ: "right-right case: a single left rotation fixes it",
  STR_ROT_ESQ_DIR:
    "left-right case: double rotation — left on the child, then right here",
  STR_ROT_DIR_ESQ:
    "right-left case: double rotation — right on the child, then left here",
  STR_REEQUILIBRADA: "the subtree fits the promise again",

  "estrutura.hashEnc": "Chained hash",
  "estrutura.hashLinear": "Open hash — linear probing",
  "estrutura.hashQuad": "Open hash — quadratic probing",
  "estrutura.hashDuplo": "Open hash — double hashing",

  "metrica.carga": "load factor",
  "metrica.colisoes": "collisions",
  "metrica.maiorCadeia": "longest chain",
  "metrica.sondagens": "probes",
  "metrica.tumulos": "tombstones",
  "metrica.baldes": "buckets",

  "log.balde": "bucket",

  STR_BALDE: "h(k) = k mod m landed on this bucket",
  STR_COLISAO: "collision: the bucket already holds another key",
  STR_SONDANDO: "taken by another key: probe the next one",
  STR_TUMULO: "tombstone — a removed cell the probe has to cross",
  STR_TABELA_CHEIA: "the table is full: the probe wrapped around without a slot",

  "estrutura.arvoreB": "B-tree",
  "metrica.grau": "degree t",
  "metrica.paginas": "pages",
  "metrica.discoLe": "disk reads",
  "metrica.discoEscreve": "disk writes",
  "log.leuPagina": "read page",
  "log.escreveuPagina": "wrote page",

  STR_PAGINA_CHEIA: "the page is full: 2t-1 keys",
  STR_DIVIDE: "split the page in two",
  STR_SOBE_CHAVE: "the middle key moves up to the parent",
  STR_EMPRESTA_ESQ: "the left sibling has a key to spare: borrow it",
  STR_EMPRESTA_DIR: "the right sibling has a key to spare: borrow it",
  STR_FUNDE: "no sibling has room to spare: the two pages become one",
  STR_DESCE_CHAVE: "the parent key moves down into the merged page",

  "estrutura.arvoreBMais": "B+ tree",

  "rotulo.raiz": "root",
  "rotulo.raizNula": "root = NULL",
  "rotulo.inicio": "head",
  "op.varrer": "scan in order",
  "metrica.folhas": "leaves",

  STR_COPIA_CHAVE:
    "the middle key is COPIED up — it stays in the leaf, which is where the data lives",
  STR_GERANDO_RUNS:
    "building the sorted runs: one block at a time, sorted inside memory",
  STR_VARRENDO:
    "scanning along the leaf chain: one page per leaf, and no internal node touched",

  "aba.comoFunciona": "How it works",

  "como.titulo": "How this works",
  "como.tese":
    "The C core draws nothing. It runs the real operation — the same stack, " +
    "the same AVL rotation, the same page split the course teaches — and " +
    "reports what it did, as a stream of fixed-size events. The page is what " +
    "animates, at whatever speed you pick, forwards or backwards. " +
    "Everything you see here follows from that separation.",

  "como.pipelineTitulo": "The path of one operation",
  "como.p1": "You click on \"push 42\".",
  "como.p2":
    "The page calls ds_call(op, a, b, c) — four integers, and nothing else " +
    "crosses in that direction.",
  "como.p3":
    "The C algorithm runs start to finish, emitting one event for every " +
    "step worth watching.",
  "como.p4":
    "The events land in a flat buffer inside the WebAssembly heap. C returns " +
    "the pointer and the count.",
  "como.p5":
    "JavaScript reads that buffer as an Int32Array over the same memory: no " +
    "copy, no parse, no text.",
  "como.p6":
    "The player applies the events one by one to the visual model, and the " +
    "canvas draws the model. Going back in time means replaying from zero.",

  "como.traceTitulo": "The events, right now",
  "como.traceTexto":
    "The table below is not a hand-written example. It was generated on this " +
    "page a moment ago: a linked stack was created, 42 was pushed, and these " +
    "are the events the second push emitted. The middle column is the C file " +
    "and line that produced each one — the same information that makes the " +
    "code panel highlight the right line while the animation runs.",
  "como.evento": "event",
  "como.origem": "from",
  "como.brutoTexto":
    "And here are the first three events as JavaScript sees them: six " +
    "integers each, read straight out of the wasm heap.",

  "como.fronteiraTitulo": "The entire boundary",
  "como.fronteiraTexto":
    "Two declarations. A four-integer function going in, and a struct with " +
    "no pointers and no strings coming out. There is no JSON anywhere, and " +
    "C never returns text: a message is an id, and the sentence you are " +
    "reading lives in the frontend's dictionary.",
  "como.cKind": "which event",
  "como.cSrc": "which .c emitted it",
  "como.cLine": "__LINE__ inside it",
  "como.cAbc": "operands; meaning depends on kind",

  "como.decisoesTitulo": "Five decisions that shape the rest",
  "como.d1": "C emits a trace, not a state",
  "como.d1Texto":
    "The obvious route would be for C to return the structure after each " +
    "operation and let the page redraw. That gives you a photograph, not an " +
    "animation — the intermediate steps vanish, and the intermediate steps " +
    "are the content. In exchange, the C stays the C the course teaches, " +
    "with one macro sprinkled where a lecture would pause to explain.",
  "como.d2": "No JSON parser in C",
  "como.d2Texto":
    "Writing a JSON parser in C would have been the dullest, buggiest part " +
    "of the project, and it would have bought nothing. Four integers one " +
    "way, a binary buffer the other.",
  "como.d3": "C never returns text",
  "como.d3Texto":
    "That is what lets the interface be bilingual without the core knowing a " +
    "language exists. And it is checked in CI: every new message in the enum " +
    "needs a translation in both dictionaries or the build complains.",
  "como.d4": "Going back in time means re-running",
  "como.d4Texto":
    "To reach step k, the model is cleared and events 0..k are applied " +
    "again. Implementing the inverse of each event would have been twice the " +
    "code and a permanent source of drift between forwards and backwards.",
  "como.d5": "The enums are generated, not maintained",
  "como.d5Texto":
    "A script reads ids.h and writes the TypeScript. Two hand-kept lists on " +
    "either side of a boundary drift eventually, and the symptom is silent: " +
    "the animation starts doing the wrong thing and nothing breaks.",

  "como.numerosTitulo": "The size of things",
  "como.numEventos": "event types in the whole vocabulary",
  "como.numMensagens": "messages, all translated in both languages",
  "como.numEstruturas": "data structures",
  "como.numAlgoritmos": "sorting algorithms",
  "como.numerosTexto":
    "The event vocabulary is deliberately small, and it is what carries " +
    "every structure: the same event that says \"top points at node 7\" says " +
    "\"head is 5\" in the array-backed queue. These numbers come from the C " +
    "enums, not from a list somebody would have to remember to update.",
  "como.repositorio": "The code, the tests and the full plan on GitHub →",

  OK: "no error",
  ERR_SEM_MEMORIA: "out of memory",
  ERR_VAZIA: "structure is empty",
  ERR_CHEIA: "structure is full",
  ERR_NAO_ENCONTRADO: "not found",
  ERR_ARG_INVALIDO: "invalid argument",
  ERR_OP_DESCONHECIDA: "unknown operation",
  ERR_SEM_SESSAO: "no session open",
};

export default en;
