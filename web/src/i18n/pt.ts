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
  "estrutura.pilhaVet": "Pilha com vetor",
  "estrutura.filaEnc": "Fila encadeada",
  "estrutura.filaVet": "Fila circular",
  "estrutura.encadeada": "encadeada",
  "estrutura.vetor": "vetor",
  "op.enfileirar": "enfileirar",
  "op.desenfileirar": "desenfileirar",
  "op.frente": "consultar frente",
  "op.capacidade": "capacidade",
  "metrica.escritas": "escritas",
  "metrica.capacidade": "capacidade",
  "metrica.ocupacao": "ocupação",
  "log.escreve": "escreve na célula",
  "log.le": "lê a célula",
  "log.marcaLivre": "célula liberada",
  "log.vetorInicia": "vetor com",
  "log.frente": "frente passa a ser",
  "log.fim": "fim passa a ser",
  "log.nulo": "NULO",

  "estrutura.listaSimples": "Lista simples",
  "estrutura.listaDupla": "Lista dupla",
  "estrutura.listaCircular": "Lista circular",

  "op.inserirInicio": "inserir no início",
  "op.inserirFim": "inserir no fim",
  "op.inserirEm": "inserir na posição",
  "op.removerEm": "remover da posição",
  "op.removerInicio": "remover do início",
  "op.buscar": "buscar",
  "op.posicao": "posição",
  "op.primeiro": "consultar o primeiro",

  "metrica.comparacoes": "comparações",

  "log.cursor": "cursor passa a ser",
  "log.inicio": "início passa a ser",

  "estrutura.comparar": "comparar as duas implementações",

  "painel.script": "Script de operações",
  "script.exemplo": "empilhar 10\nempilhar 20\ndesempilhar\n# ou: i 1 2 3",
  "script.ajuda":
    "Uma operação por linha, ou separadas por vírgula. Ctrl+Enter roda.",
  "script.rodar": "rodar script",
  "script.linha": "linha",

  /* mensagens emitidas pelo C (EV_MSG carrega o id, nunca a frase) */
  STR_NENHUMA: "",
  STR_PING: "o núcleo respondeu",
  STR_PILHA_VAZIA: "a pilha está vazia",
  STR_PILHA_CHEIA: "a pilha está cheia — n == capacidade",
  STR_FILA_VAZIA: "a fila está vazia",
  STR_FILA_CHEIA: "a fila está cheia — n == capacidade",
  STR_DEU_VOLTA: "o índice deu a volta",
  STR_LISTA_VAZIA: "a lista está vazia",
  STR_POSICAO_INVALIDA: "posição fora da lista",
  STR_ACHOU: "encontrado",
  STR_NAO_ACHOU: "não está na lista",
  STR_ANDANDO: "andando até a posição",

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
