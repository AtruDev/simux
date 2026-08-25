import type { Dicionario } from "./index";

/* Traduções em português. A interface é bilíngue; o código-fonte em C não —
 * ele é exibido como está, nos dois idiomas. */
const pt: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "Simulador visual de estruturas de dados. O núcleo é C compilado para " +
    "WebAssembly; a página anima o que ele executou.",
  "app.botaoPing": "Chamar OP_PING no C",
  "app.carregando": "carregando o módulo…",
  "app.semEventos": "Nenhum evento ainda.",
  "app.tabelaEvento": "evento",
  "app.tabelaOrigem": "origem",
  "app.tabelaLinha": "linha",
  "app.tabelaMensagem": "mensagem",
  "app.trocarIdioma": "English",
  "app.erro": "erro",

  /* mensagens emitidas pelo C (EV_MSG carrega o id, nunca a frase) */
  STR_NENHUMA: "",
  STR_PING: "o núcleo respondeu",

  /* códigos de status devolvidos pelo core */
  OK: "sem erro",
  ERR_SEM_MEMORIA: "sem memória",
  ERR_VAZIA: "estrutura vazia",
  ERR_CHEIA: "estrutura cheia",
  ERR_NAO_ENCONTRADO: "não encontrado",
  ERR_ARG_INVALIDO: "argumento inválido",
  ERR_OP_DESCONHECIDA: "operação desconhecida",
  ERR_SEM_SESSAO: "nenhuma sessão aberta",
};

export default pt;
