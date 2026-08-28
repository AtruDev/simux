/* web/src/i18n/index.ts — a camada de tradução.
 *
 * Nenhum texto voltado ao usuário aparece literal num componente, desde o
 * primeiro commit: retrofitar isso significaria caçar string em cinquenta
 * arquivos. Vale também para rótulo desenhado no canvas, que é o lugar mais
 * fácil de esquecer.
 *
 * As chaves de mensagem e de erro vêm de ops.ts, que é gerado do ids.h. É
 * isso que faz o TypeScript cobrar tradução para todo STR_ novo, em vez de a
 * interface mostrar undefined. */

import type { ErrChave, StrChave } from "../core/ops";

import en from "./en";
import pt from "./pt";

/** Chaves da interface. As de mensagem e erro vêm do C. */
export type ChaveUI =
  | "app.titulo"
  | "app.descricao"
  | "app.trocarIdioma"
  | "app.carregando"
  | "app.erro"
  | "app.truncado"
  | "painel.operacoes"
  | "painel.codigo"
  | "painel.metricas"
  | "painel.log"
  | "estrutura.titulo"
  | "estrutura.pilhaEnc"
  | "op.valor"
  | "op.push"
  | "op.pop"
  | "op.topo"
  | "op.limpar"
  | "op.aleatorio"
  | "transporte.inicio"
  | "transporte.anterior"
  | "transporte.tocar"
  | "transporte.pausar"
  | "transporte.proximo"
  | "transporte.fim"
  | "transporte.velocidade"
  | "transporte.passo"
  | "metrica.tamanho"
  | "metrica.alocacoes"
  | "metrica.nos"
  | "metrica.eventos"
  | "log.vazio"
  | "log.noCriado"
  | "log.noLiberado"
  | "log.aresta"
  | "log.ponteiro"
  | "log.visita"
  | "log.saiVisita"
  | "log.contador"
  | "log.nulo"
  | "estrutura.pilhaVet"
  | "estrutura.filaEnc"
  | "estrutura.filaVet"
  | "estrutura.encadeada"
  | "estrutura.vetor"
  | "op.enfileirar"
  | "op.desenfileirar"
  | "op.frente"
  | "op.capacidade"
  | "metrica.escritas"
  | "metrica.capacidade"
  | "metrica.ocupacao"
  | "log.escreve"
  | "log.le"
  | "log.marcaLivre"
  | "log.vetorInicia"
  | "log.frente"
  | "log.fim"
  | "painel.script"
  | "script.exemplo"
  | "script.ajuda"
  | "script.rodar"
  | "script.linha"
  | "estrutura.comparar"
  | "estrutura.listaSimples"
  | "estrutura.listaDupla"
  | "estrutura.listaCircular"
  | "op.inserirInicio"
  | "op.inserirFim"
  | "op.inserirEm"
  | "op.removerEm"
  | "op.removerInicio"
  | "op.buscar"
  | "op.posicao"
  | "op.primeiro"
  | "metrica.comparacoes"
  | "log.cursor"
  | "log.inicio"
  | "aba.estruturas"
  | "aba.ordenacao"
  | "painel.algoritmo"
  | "painel.cena"
  | "painel.empirico"
  | "painel.legenda"
  | "painel.fase"
  | "alg.bolha"
  | "alg.selecao"
  | "alg.insercao"
  | "alg.shell"
  | "alg.quick"
  | "alg.merge"
  | "alg.externa"
  | "ordem.externa"
  | "ord.memoria"
  | "ord.memoriaPorque"
  | "metrica.passadas"
  | "ordem.quadratica"
  | "ordem.linearitmica"
  | "ordem.shell"
  | "dist.aleatorio"
  | "dist.aleatorioPorque"
  | "dist.quaseOrdenado"
  | "dist.quaseOrdenadoPorque"
  | "dist.inverso"
  | "dist.inversoPorque"
  | "dist.poucosDistintos"
  | "dist.poucosDistintosPorque"
  | "dist.ordenado"
  | "dist.ordenadoPorque"
  | "dist.manual"
  | "dist.manualPorque"
  | "ord.tamanho"
  | "ord.semente"
  | "ord.distribuicao"
  | "ord.gerar"
  | "ord.novaSemente"
  | "ord.ordenar"
  | "ord.corrida"
  | "ord.valores"
  | "ord.valoresAjuda"
  | "ord.semFase"
  | "legenda.comparando"
  | "legenda.escrita"
  | "legenda.ordenado"
  | "legenda.pivo"
  | "legenda.auxiliar"
  | "legenda.fora"
  | "legenda.cursores"
  | "empirico.rodar"
  | "empirico.rodando"
  | "empirico.metrica"
  | "empirico.comparacoes"
  | "empirico.escritas"
  | "empirico.vazio"
  | "empirico.explica"
  | "empirico.teoria"
  | "empirico.limite"
  | "log.compara"
  | "log.emMaos"
  | "log.troca"
  | "log.faixa"
  | "log.auxInicia"
  | "log.auxEscreve"
  | "log.fase"
  | "log.marcaOrdenado"
  | "log.marcaPivo"
  | "estrutura.buscaSeq"
  | "estrutura.buscaBin"
  | "op.inserirOrdenado"
  | "op.removerMenor"
  | "op.menor"
  | "legenda.chave"
  | "legenda.faixaViva"
  | "legenda.achado"
  | "estrutura.abb"
  | "op.inserir"
  | "op.removerValor"
  | "op.percurso"
  | "perc.emOrdem"
  | "perc.preOrdem"
  | "perc.posOrdem"
  | "metrica.altura"
  | "metrica.alturaIdeal"
  | "log.raiz"
  | "log.esquerda"
  | "log.direita"
  | "estrutura.avl"
  | "metrica.rotacoes"
  | "estrutura.hashEnc"
  | "estrutura.hashLinear"
  | "estrutura.hashQuad"
  | "estrutura.hashDuplo"
  | "metrica.carga"
  | "metrica.colisoes"
  | "metrica.maiorCadeia"
  | "metrica.sondagens"
  | "metrica.tumulos"
  | "metrica.baldes"
  | "log.balde"
  | "estrutura.arvoreB"
  | "estrutura.arvoreBMais"
  | "aba.comoFunciona"
  | "como.titulo"
  | "como.tese"
  | "como.pipelineTitulo"
  | "como.p1"
  | "como.p2"
  | "como.p3"
  | "como.p4"
  | "como.p5"
  | "como.p6"
  | "como.traceTitulo"
  | "como.traceTexto"
  | "como.evento"
  | "como.origem"
  | "como.brutoTexto"
  | "como.fronteiraTitulo"
  | "como.fronteiraTexto"
  | "como.cKind"
  | "como.cSrc"
  | "como.cLine"
  | "como.cAbc"
  | "como.decisoesTitulo"
  | "como.d1"
  | "como.d1Texto"
  | "como.d2"
  | "como.d2Texto"
  | "como.d3"
  | "como.d3Texto"
  | "como.d4"
  | "como.d4Texto"
  | "como.d5"
  | "como.d5Texto"
  | "como.numerosTitulo"
  | "como.numEventos"
  | "como.numMensagens"
  | "como.numEstruturas"
  | "como.numAlgoritmos"
  | "como.numerosTexto"
  | "como.repositorio"
  | "rotulo.raiz"
  | "rotulo.raizNula"
  | "rotulo.inicio"
  | "op.varrer"
  | "metrica.folhas"
  | "metrica.grau"
  | "metrica.paginas"
  | "metrica.discoLe"
  | "metrica.discoEscreve"
  | "log.leuPagina"
  | "log.escreveuPagina";

export type Chave = ChaveUI | StrChave | ErrChave;

/** Record exige a chave inteira: faltar uma é erro de compilação. */
export type Dicionario = Record<Chave, string>;

export type Idioma = "pt" | "en";

const DICIONARIOS: Record<Idioma, Dicionario> = { pt, en };
const ARMAZENAMENTO = "simux.idioma";

function ehIdioma(v: string | null): v is Idioma {
  return v === "pt" || v === "en";
}

/* Ordem: ?lang= na URL, depois a escolha salva, depois o navegador. A URL vem
 * primeiro para um link compartilhado abrir no idioma de quem enviou. */
function detectar(): Idioma {
  const daUrl = new URLSearchParams(window.location.search).get("lang");
  if (ehIdioma(daUrl)) {
    return daUrl;
  }
  const salvo = window.localStorage.getItem(ARMAZENAMENTO);
  if (ehIdioma(salvo)) {
    return salvo;
  }
  return window.navigator.language.toLowerCase().startsWith("pt") ? "pt" : "en";
}

let idioma: Idioma = detectar();

export function idiomaAtual(): Idioma {
  return idioma;
}

export function definirIdioma(novo: Idioma): void {
  idioma = novo;
  window.localStorage.setItem(ARMAZENAMENTO, novo);
  document.documentElement.lang = novo === "pt" ? "pt-BR" : "en";
}

export function t(chave: Chave): string {
  return DICIONARIOS[idioma][chave];
}
