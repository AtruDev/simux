import type { Dicionario } from "./index";

/* Traduções em português. A interface é bilíngue; o código-fonte em C não —
 * ele é exibido como está, nos dois idiomas. */
const pt: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "Simulador visual de estruturas de dados. O núcleo é C compilado para " +
    "WebAssembly; a página anima o que ele executou.",
  "app.trocarIdioma": "English",
  "app.carregando": "carregando o núcleo…",
  "app.erro": "erro",
  "app.truncado": "trace truncado — eventos foram perdidos",

  "painel.operacoes": "Operações",
  "painel.codigo": "Código-fonte",
  "painel.metricas": "Métricas",
  "painel.log": "Log",

  "estrutura.titulo": "Estrutura",
  "estrutura.pilhaEnc": "Pilha encadeada",

  "op.valor": "valor",
  "op.push": "empilhar",
  "op.pop": "desempilhar",
  "op.topo": "consultar topo",
  "op.limpar": "limpar",
  "op.aleatorio": "aleatório",

  "transporte.inicio": "voltar ao início",
  "transporte.anterior": "passo anterior",
  "transporte.tocar": "tocar",
  "transporte.pausar": "pausar",
  "transporte.proximo": "próximo passo",
  "transporte.fim": "ir para o fim",
  "transporte.velocidade": "velocidade",
  "transporte.passo": "passo",

  "metrica.tamanho": "tamanho",
  "metrica.alocacoes": "alocações",
  "metrica.nos": "nós na tela",
  "metrica.eventos": "eventos",

  "log.vazio": "Nada aconteceu ainda.",
  "log.noCriado": "nó criado",
  "log.noLiberado": "nó liberado",
  "log.aresta": "aponta para",
  "log.ponteiro": "topo passa a ser",
  "log.visita": "olhando o nó",
  "log.saiVisita": "solta o nó",
  "log.contador": "contador",
  "log.nulo": "NULO",

  /* mensagens emitidas pelo C (EV_MSG carrega o id, nunca a frase) */
  STR_NENHUMA: "",
  STR_PING: "o núcleo respondeu",
  STR_PILHA_VAZIA: "a pilha está vazia",

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
