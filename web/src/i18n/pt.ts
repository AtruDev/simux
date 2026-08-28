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

  "aba.estruturas": "Estruturas",
  "aba.ordenacao": "Ordenação",

  "painel.algoritmo": "Algoritmo",
  "painel.cena": "Vetor inicial",
  "painel.empirico": "Modo empírico",
  "painel.legenda": "Legenda",
  "painel.fase": "Fase",

  "alg.bolha": "Bolha",
  "alg.selecao": "Seleção",
  "alg.insercao": "Inserção",
  "alg.shell": "Shellsort",
  "alg.quick": "Quicksort",
  "alg.merge": "Mergesort",

  "ordem.quadratica": "O(n²)",
  "ordem.linearitmica": "O(n log n)",
  "ordem.shell": "entre O(n log² n) e O(n²)",

  "dist.aleatorio": "aleatório",
  "dist.aleatorioPorque": "o caso médio, que é o que as tabelas medem",
  "dist.quaseOrdenado": "quase ordenado",
  "dist.quaseOrdenadoPorque":
    "aqui a inserção ganha do quicksort, e a bolha sai numa passada",
  "dist.inverso": "inversamente ordenado",
  "dist.inversoPorque": "o pior caso da inserção e o máximo de trocas da bolha",
  "dist.poucosDistintos": "poucos valores distintos",
  "dist.poucosDistintosPorque":
    "derruba o quicksort de pivô ingênuo: quase tudo cai de um lado só",
  "dist.ordenado": "já ordenado",
  "dist.ordenadoPorque":
    "a bolha para na primeira passada; a seleção custa o mesmo de sempre",
  "dist.manual": "manual",
  "dist.manualPorque": "os valores do enunciado, colados como estão",

  "ord.tamanho": "tamanho",
  "ord.semente": "semente",
  "ord.distribuicao": "distribuição",
  "ord.gerar": "gerar vetor",
  "ord.novaSemente": "sortear semente",
  "ord.ordenar": "ordenar",
  "ord.corrida": "modo corrida",
  "ord.valores": "valores",
  "ord.valoresAjuda": "Separe por vírgula, espaço ou quebra de linha.",
  "ord.semFase": "parado",

  "legenda.comparando": "comparando",
  "legenda.escrita": "acabou de escrever",
  "legenda.ordenado": "já no lugar",
  "legenda.pivo": "pivô",
  "legenda.auxiliar": "memória auxiliar (tracejada)",
  "legenda.fora": "fora do trecho ativo",
  "legenda.cursores": "cursores i, j e mín",

  "empirico.rodar": "medir",
  "empirico.rodando": "medindo…",
  "empirico.metrica": "métrica",
  "empirico.comparacoes": "comparações",
  "empirico.escritas": "escritas",
  "empirico.vazio": "Nada medido ainda.",
  "empirico.explica":
    "Cada ponto é uma execução de verdade, com o trace desligado. As linhas " +
    "finas são as curvas teóricas ancoradas no primeiro ponto de cada " +
    "algoritmo — se a medida acompanha a curva, a teoria bate.",
  "empirico.teoria": "curvas teóricas",
  "empirico.limite": "cada algoritmo vai até onde ainda faz sentido medir",

  "log.compara": "compara",
  "log.emMaos": "com o valor em mãos",
  "log.troca": "troca",
  "log.faixa": "trecho ativo",
  "log.auxInicia": "auxiliar com",
  "log.auxEscreve": "escreve no auxiliar",
  "log.fase": "fase",
  "log.marcaOrdenado": "no lugar",
  "log.marcaPivo": "vira pivô",

  STR_ORDENADO: "vetor ordenado",
  STR_SEM_TROCAS: "nenhuma troca nesta passada — já está ordenado",
  STR_PASSADA: "passada",
  STR_PROCURANDO_MIN: "procurando o menor do trecho",
  STR_DESLOCANDO: "abrindo espaço à direita",
  STR_GAP: "passada com gap",
  STR_PARTICIONANDO: "particionando",
  STR_DIVIDINDO: "dividindo",
  STR_INTERCALANDO: "intercalando",

  "estrutura.buscaSeq": "Busca sequencial",
  "estrutura.buscaBin": "Busca binária",
  "op.inserirOrdenado": "inserir em ordem",
  "op.removerMenor": "remover o menor",
  "op.menor": "consultar o menor",
  "legenda.chave": "chave procurada",
  "legenda.faixaViva": "trecho que ainda pode conter a chave",
  "legenda.achado": "encontrado",

  STR_VETOR_CHEIO: "o vetor está cheio — n == capacidade",
  STR_DESCARTA_ESQ: "o procurado é maior: a metade de baixo sai",
  STR_DESCARTA_DIR: "o procurado é menor: a metade de cima sai",

  "estrutura.abb": "Árvore binária de busca",
  "op.inserir": "inserir",
  "op.removerValor": "remover o valor",
  "op.percurso": "percorrer",
  "perc.emOrdem": "em ordem",
  "perc.preOrdem": "pré-ordem",
  "perc.posOrdem": "pós-ordem",
  "metrica.altura": "altura",
  "metrica.alturaIdeal": "altura mínima",

  STR_VAI_ESQ: "o procurado é menor: desce à esquerda",
  STR_VAI_DIR: "o procurado é maior: desce à direita",
  STR_JA_EXISTE: "o valor já está na árvore",
  STR_CASO_FOLHA: "caso 1: o nó é folha — sai sem substituto",
  STR_CASO_UM_FILHO: "caso 2: o filho único sobe para o lugar dele",
  STR_CASO_DOIS_FILHOS: "caso 3: dois filhos — quem sobe é o sucessor em ordem",
  STR_PROCURA_SUCESSOR:
    "o sucessor é o menor da subárvore direita: à direita uma vez, e à esquerda o resto",
  STR_SUBSTITUI: "o valor do sucessor toma o lugar, e o sucessor é que sai",
  STR_PERCURSO: "percorrendo",

  "log.raiz": "raiz passa a ser",
  "log.esquerda": "esquerda →",
  "log.direita": "direita →",

  "estrutura.avl": "Árvore AVL",
  "metrica.rotacoes": "rotações",

  STR_DESBALANCEOU: "o fator de balanceamento estourou: |FB| > 1",
  STR_ROT_DIR: "caso esquerda-esquerda: uma rotação à direita resolve",
  STR_ROT_ESQ: "caso direita-direita: uma rotação à esquerda resolve",
  STR_ROT_ESQ_DIR:
    "caso esquerda-direita: rotação dupla — à esquerda no filho, depois à direita aqui",
  STR_ROT_DIR_ESQ:
    "caso direita-esquerda: rotação dupla — à direita no filho, depois à esquerda aqui",
  STR_REEQUILIBRADA: "a subárvore voltou a caber na promessa",

  "estrutura.hashEnc": "Hash encadeado",
  "estrutura.hashLinear": "Hash aberto — sondagem linear",
  "estrutura.hashQuad": "Hash aberto — sondagem quadrática",
  "estrutura.hashDuplo": "Hash aberto — sondagem dupla",

  "metrica.carga": "fator de carga",
  "metrica.colisoes": "colisões",
  "metrica.maiorCadeia": "maior cadeia",
  "metrica.sondagens": "sondagens",
  "metrica.tumulos": "túmulos",
  "metrica.baldes": "baldes",

  "log.balde": "balde",

  STR_BALDE: "h(k) = k mod m caiu neste balde",
  STR_COLISAO: "colisão: o balde já está ocupado por outra chave",
  STR_SONDANDO: "ocupada por outra chave: sonda a próxima",
  STR_TUMULO: "túmulo — célula removida, que a sondagem atravessa",
  STR_TABELA_CHEIA: "a tabela está cheia: a sondagem deu a volta sem achar lugar",

  "estrutura.arvoreB": "Árvore B",
  "metrica.grau": "grau t",
  "metrica.paginas": "páginas",
  "metrica.discoLe": "leituras de disco",
  "metrica.discoEscreve": "escritas em disco",
  "log.leuPagina": "leu a página",
  "log.escreveuPagina": "escreveu a página",

  STR_PAGINA_CHEIA: "a página está cheia: 2t-1 chaves",
  STR_DIVIDE: "divide a página em duas",
  STR_SOBE_CHAVE: "a chave do meio sobe para o pai",
  STR_EMPRESTA_ESQ: "o irmão da esquerda tem chave sobrando: empresta uma",
  STR_EMPRESTA_DIR: "o irmão da direita tem chave sobrando: empresta uma",
  STR_FUNDE: "nenhum irmão tem folga: as duas páginas viram uma",
  STR_DESCE_CHAVE: "a chave do pai desce para o meio da página nova",

  "estrutura.arvoreBMais": "Árvore B+",

  /* Rótulos desenhados no canvas. Passam pelo i18n como qualquer outro
   * texto voltado ao usuário — o canvas é a parte mais fácil de esquecer. */
  "rotulo.raiz": "raiz",
  "rotulo.raizNula": "raiz = NULL",
  "rotulo.inicio": "início",
  "op.varrer": "varrer em ordem",
  "metrica.folhas": "folhas",

  STR_COPIA_CHAVE:
    "a chave do meio é COPIADA para o pai — e continua na folha, porque é lá que o dado mora",
  STR_VARRENDO:
    "varrendo pela corrente de folhas: uma página por folha, e nenhum nó interno",

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
