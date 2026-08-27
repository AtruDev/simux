import type { Dicionario } from "./index";

const en: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "A visual simulator for data structures. The core is C compiled to " +
    "WebAssembly; the page animates what it executed.",
  "app.trocarIdioma": "Português",
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
