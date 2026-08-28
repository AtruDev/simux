import type { Dicionario } from "./index";

/* Traduções em português. A interface é bilíngue; o código-fonte em C não —
 * ele é exibido como está, nos dois idiomas. */
const pt: Dicionario = {
  "app.titulo": "simux",
  "app.descricao":
    "Simulador visual de estruturas de dados. O núcleo é C compilado para " +
    "WebAssembly; a página anima o que ele executou.",
  "app.trocarIdioma": "English",
  "app.copiarLink": "copiar link",
  "app.linkCopiado": "copiado",
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
  "alg.externa": "Intercalação externa",
  "ordem.externa": "1 + ⌈log₂(n/k)⌉ passadas",
  "ord.memoria": "memória k",
  "ord.memoriaPorque":
    "Quantos registros cabem na RAM. O resto do vetor está \"no disco\": " +
    "cada passada lê e escreve o arquivo inteiro, e dobrar k tira uma passada.",
  "metrica.passadas": "passadas",

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
  STR_GERANDO_RUNS:
    "gerando os blocos ordenados: um bloco por vez, ordenado dentro da memória",
  STR_VARRENDO:
    "varrendo pela corrente de folhas: uma página por folha, e nenhum nó interno",

  /* ---- a aba "como funciona" ------------------------------------------- *
   * A página que o plano põe acima de qualquer estrutura extra: quem abre o
   * link vê a animação, e não tem como saber o que está por baixo dela.     */
  "aba.comoFunciona": "Como funciona",

  "como.titulo": "Como isto funciona",
  "como.tese":
    "O núcleo em C não desenha nada. Ele executa a operação de verdade — a " +
    "mesma pilha, a mesma rotação de AVL, a mesma divisão de página que a " +
    "matéria ensina — e vai contando o que fez, num fluxo de eventos de " +
    "tamanho fixo. Quem anima é a página, e ela anima na velocidade que você " +
    "escolher, para a frente e para trás. Tudo o que você vê aqui é " +
    "consequência dessa separação.",

  "como.pipelineTitulo": "O caminho de uma operação",
  "como.p1": "Você clica em «empilhar 42».",
  "como.p2":
    "A página chama ds_call(op, a, b, c) — quatro inteiros, e nada mais " +
    "atravessa nesse sentido.",
  "como.p3":
    "O algoritmo em C roda inteiro, do começo ao fim, emitindo um evento a " +
    "cada passo que valha a pena assistir.",
  "como.p4":
    "Os eventos ficam num buffer plano dentro da heap do WebAssembly. O C " +
    "devolve o ponteiro e a quantidade.",
  "como.p5":
    "O JavaScript lê esse buffer como um Int32Array sobre a mesma memória: " +
    "sem cópia, sem parse, sem texto.",
  "como.p6":
    "O Player aplica os eventos um a um ao modelo visual, e o canvas desenha " +
    "o modelo. Voltar no tempo é reaplicar do zero.",

  "como.traceTitulo": "Os eventos, agora",
  "como.traceTexto":
    "A tabela abaixo não é um exemplo escrito à mão. Ela foi gerada nesta " +
    "página, há um instante: uma pilha encadeada foi criada, o 42 foi " +
    "empilhado, e estes são os eventos que o segundo empilhar emitiu. A " +
    "coluna do meio é o arquivo e a linha do C que emitiu cada um — é a " +
    "mesma informação que faz o painel de código destacar a linha certa " +
    "enquanto a animação roda.",
  "como.evento": "evento",
  "como.origem": "origem",
  "como.brutoTexto":
    "E estes são os três primeiros eventos como o JavaScript os enxerga: " +
    "seis inteiros cada um, lidos direto da heap do wasm.",

  "como.fronteiraTitulo": "A fronteira inteira",
  "como.fronteiraTexto":
    "São duas declarações. Uma função de quatro inteiros para dentro, e uma " +
    "struct sem ponteiro e sem string para fora. Não existe JSON em lugar " +
    "nenhum, e o C nunca devolve texto: uma mensagem é um id, e a frase que " +
    "você está lendo mora no dicionário do frontend.",
  "como.cKind": "que evento é",
  "como.cSrc": "qual .c emitiu",
  "como.cLine": "__LINE__ dentro dele",
  "como.cAbc": "operandos; o significado depende do kind",

  "como.decisoesTitulo": "Cinco decisões que definem o resto",
  "como.d1": "O C emite um trace, e não um estado",
  "como.d1Texto":
    "O caminho óbvio seria o C devolver a estrutura depois de cada operação " +
    "e a página redesenhar. Isso dá uma fotografia, não uma animação — os " +
    "passos intermediários somem, e são eles o conteúdo. Em troca, o C " +
    "continua sendo o C da matéria, com uma macro espalhada nos pontos onde " +
    "a aula pararia para explicar.",
  "como.d2": "Zero parser de JSON no C",
  "como.d2Texto":
    "Escrever um parser de JSON em C teria sido a parte mais chata e mais " +
    "bugada do projeto, e não compraria nada. Quatro inteiros para um lado, " +
    "um buffer binário para o outro.",
  "como.d3": "O C nunca devolve texto",
  "como.d3Texto":
    "É o que deixa a interface ser bilíngue sem o núcleo saber que idioma " +
    "existe. E é verificado no CI: toda mensagem nova no enum precisa de " +
    "tradução nos dois dicionários, senão o build reclama.",
  "como.d4": "Voltar no tempo é reexecutar",
  "como.d4Texto":
    "Para chegar ao passo k, o modelo é zerado e os eventos 0..k são " +
    "aplicados de novo. Implementar o inverso de cada evento seria o dobro " +
    "do código e uma fonte permanente de divergência entre ir e voltar.",
  "como.d5": "Os enums são gerados, não mantidos",
  "como.d5Texto":
    "Um script lê o ids.h e escreve o TypeScript. Duas listas mantidas à " +
    "mão dos dois lados de uma fronteira dessincronizam mais cedo ou mais " +
    "tarde, e o sintoma é mudo: a animação passa a fazer a coisa errada e " +
    "nada quebra.",

  "como.numerosTitulo": "O tamanho das coisas",
  "como.numEventos": "tipos de evento no vocabulário inteiro",
  "como.numMensagens": "mensagens, todas traduzidas nos dois idiomas",
  "como.numEstruturas": "estruturas de dados",
  "como.numAlgoritmos": "algoritmos de ordenação",
  "como.numerosTexto":
    "O vocabulário de eventos é pequeno de propósito, e é ele que sustenta " +
    "todas as estruturas: o mesmo evento que diz «o topo aponta para o nó " +
    "7» diz «início vale 5» na fila com vetor. Estes números saem dos enums " +
    "do C, e não de uma lista que alguém teria que lembrar de atualizar.",
  "como.repositorio": "O código, os testes e o plano completo no GitHub →",

  /* ---- o painel que explica a estrutura na tela ------------------------ *
   * Nem definição nem fórmula: para que serve, e o que ela cobra por isso. */
  "sobre.titulo": "Sobre",
  "sobre.operacao": "operação",
  "sobre.medio": "médio",
  "sobre.pior": "pior",
  "sobre.quando": "Quando usar.",
  "sobre.pega": "A pega.",

  "sobre.pilhaEnc.quando":
    "Quando só o último a entrar interessa: desfazer, casar parênteses, a " +
    "pilha de chamadas de um programa. Cresce até a memória acabar.",
  "sobre.pilhaEnc.pega":
    "Cada elemento custa um malloc e um ponteiro além do dado — 16 bytes " +
    "para guardar 4. É o preço de não ter capacidade fixa, e é o que a " +
    "versão com vetor não paga.",

  "sobre.pilhaVet.quando":
    "A mesma pilha, quando o tamanho máximo é conhecido: o vetor é " +
    "reservado de uma vez, e empilhar vira escrever numa posição.",
  "sobre.pilhaVet.pega":
    "Ela enche. O overflow não é falha de implementação: é a consequência " +
    "de trocar alocação por memória contígua, e é o erro que a encadeada " +
    "não comete.",

  "sobre.filaEnc.quando":
    "Quando a ordem de chegada é a ordem de atendimento: fila de impressão, " +
    "mensagens, a busca em largura de um grafo.",
  "sobre.filaEnc.pega":
    "Desenfileirar sem liberar o nó vaza memória a cada operação. A fila " +
    "continua funcionando, e é isso que faz o vazamento demorar a aparecer.",

  "sobre.filaVet.quando":
    "A mesma fila com capacidade fixa, sem alocar nada durante o uso — e é " +
    "por isso que o fim dá a volta por cima do começo.",
  "sobre.filaVet.pega":
    "Com o vetor dando a volta, «fim igual a início» quer dizer cheia e " +
    "vazia ao mesmo tempo. Só o contador separa as duas, e é por isso que " +
    "ele está sempre na tela.",

  "sobre.listaSimples.quando":
    "Quando o número de elementos muda muito e o acesso é sequencial: " +
    "inserir e remover em qualquer ponto não desloca nada.",
  "sobre.listaSimples.pega":
    "Chegar à posição k custa k passos. A lista não tem índice — a posição " +
    "é uma caminhada, e o contador de comparações mostra o preço dela.",

  "sobre.listaDupla.quando":
    "Quando se anda nos dois sentidos, ou quando se remove um nó que já se " +
    "tem em mãos: com o ponteiro para trás, não é preciso procurar o " +
    "anterior.",
  "sobre.listaDupla.pega":
    "Um ponteiro a mais por nó, e duas religações por operação em vez de " +
    "uma. Esquecer uma delas dá uma lista que anda certo para a frente e " +
    "mente para trás.",

  "sobre.listaCircular.quando":
    "Quando não há fim: rodízio de turnos, buffer de reprodução, a rodada " +
    "de um jogo. Do último se chega ao primeiro sem caso especial.",
  "sobre.listaCircular.pega":
    "Percorrer sem uma condição de parada é laço infinito. O fim deixou de " +
    "ser NULL e passou a ser «voltei ao começo», e quem esquece isso trava " +
    "a página.",

  "sobre.buscaSeq.quando":
    "Quando o vetor é pequeno, ou quando ele não está ordenado. Ela não " +
    "exige nada em troca — é a única que funciona em qualquer vetor.",
  "sobre.buscaSeq.pega":
    "Cresce com n. Em mil elementos são até mil comparações, e é esse " +
    "número ao lado do da busca binária que faz o argumento inteiro.",

  "sobre.buscaBin.quando":
    "Quando o vetor está ordenado e vai ser buscado muitas vezes: cada " +
    "comparação joga fora metade do que restava.",
  "sobre.buscaBin.pega":
    "Ela exige a ordem, e manter a ordem custa: cada inserção desloca " +
    "metade do vetor. A busca fica barata, a escrita não — e o painel " +
    "mostra as duas.",

  "sobre.abb.quando":
    "Quando se quer buscar, inserir e remover mantendo a ordem, e as " +
    "chaves chegam embaralhadas. O percurso em ordem sai crescente de graça.",
  "sobre.abb.pega":
    "Inserir em ordem crescente degenera a árvore numa lista, e a busca " +
    "volta a ser O(n). Experimente: insira 1 2 3 4 5 e olhe a altura. É " +
    "esse caso que a AVL existe para consertar.",

  "sobre.avl.quando":
    "Quando o pior caso importa: ela garante altura logarítmica aconteça o " +
    "que acontecer com a ordem de entrada.",
  "sobre.avl.pega":
    "O equilíbrio é pago em rotações a cada inserção e remoção. Se as " +
    "escritas são muitas e as buscas poucas, a garantia pode não valer o " +
    "preço — e o contador de rotações é onde ele aparece.",

  "sobre.hashEnc.quando":
    "Quando só interessa «está lá ou não» e a ordem não importa. É a " +
    "estrutura mais rápida do quadro para isso, e a única sem ordem nenhuma.",
  "sobre.hashEnc.pega":
    "O O(1) é médio, não garantido: com um m mal escolhido, todas as " +
    "chaves caem no mesmo balde e a tabela vira uma lista. Troque o m de 8 " +
    "para 7 e olhe a maior cadeia.",

  "sobre.hashLinear.quando":
    "Quando não se quer alocar nada por elemento: tudo mora dentro do " +
    "vetor, e o que sobra é o custo de achar a próxima célula livre.",
  "sobre.hashLinear.pega":
    "Chaves vizinhas formam blocos, e cada sondagem que cai num bloco " +
    "precisa atravessá-lo inteiro. É o agrupamento primário, e ele piora " +
    "rápido conforme o fator de carga sobe.",

  "sobre.hashQuad.quando":
    "Quando a sondagem linear está agrupando demais: os saltos crescem ao " +
    "quadrado e espalham os blocos.",
  "sobre.hashQuad.pega":
    "Duas chaves que caem no mesmo balde ainda seguem exatamente a mesma " +
    "sequência de saltos — o agrupamento secundário. E a sondagem pode " +
    "recusar um elemento com a tabela ainda tendo espaço livre.",

  "sobre.hashDuplo.quando":
    "Quando o fator de carga é alto e as duas sondagens acima já " +
    "agrupam: o passo do salto passa a depender da chave, então duas " +
    "chaves no mesmo balde tomam caminhos diferentes.",
  "sobre.hashDuplo.pega":
    "O segundo hash tem que ser primo com o tamanho da tabela, senão a " +
    "sondagem não visita todas as células. O que falha é a inserção, e não " +
    "a busca — que é o tipo de defeito que demora a ser notado.",

  "sobre.arvoreB.quando":
    "Quando os dados estão em disco. Muitas chaves por página e poucos " +
    "níveis: cada nível a menos é um acesso a menos, e um acesso custa " +
    "milhões de vezes mais que uma comparação.",
  "sobre.arvoreB.pega":
    "Ler tudo em ordem sobe e desce, e relê a página do pai a cada volta. " +
    "A ordem de grandeza é a mesma da B+, mas a constante não: com t = 3 e " +
    "500 chaves são 413 páginas contra 166.",

  "sobre.arvoreBMais.quando":
    "Quando as consultas devolvem FAIXAS, e não uma linha só. Achou a " +
    "primeira chave, o resto é seguir o elo entre as folhas — e é por isso " +
    "que quase todo índice de banco de dados é B+.",
  "sobre.arvoreBMais.pega":
    "A busca pontual desce sempre até a folha, mesmo quando a chave " +
    "aparece num nó interno: ali é roteiro, não dado. Trocou-se o melhor " +
    "caso da busca por uma varredura barata.",

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
