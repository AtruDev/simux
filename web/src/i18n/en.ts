import type { Dicionario } from "./index";

const en: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "A visual simulator for data structures. The core is C compiled to " +
    "WebAssembly; the page animates what it executed.",
  "app.botaoPing": "Call OP_PING in C",
  "app.carregando": "loading the module…",
  "app.semEventos": "No events yet.",
  "app.tabelaEvento": "event",
  "app.tabelaOrigem": "source",
  "app.tabelaLinha": "line",
  "app.tabelaMensagem": "message",
  "app.trocarIdioma": "Português",
  "app.erro": "error",

  STR_NENHUMA: "",
  STR_PING: "the core answered",

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
