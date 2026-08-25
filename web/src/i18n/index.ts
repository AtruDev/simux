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
  | "app.botaoPing"
  | "app.carregando"
  | "app.semEventos"
  | "app.tabelaEvento"
  | "app.tabelaOrigem"
  | "app.tabelaLinha"
  | "app.tabelaMensagem"
  | "app.trocarIdioma"
  | "app.erro";

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
