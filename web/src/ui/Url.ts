/* O estado da tela na barra de endereços.
 *
 * O plano chama isto de "feature barata que impressiona muito", e a parte
 * barata é verdade: o que o link precisa carregar é o que a pessoa escolheu —
 * a aba, o idioma, a estrutura, a cena — e tudo isso já existe como estado do
 * React. O que ele NÃO carrega é o desenho: o link diz "árvore AVL, insira
 * 1 2 3 4 5", e quem reconstrói a tela é o núcleo, rodando de novo. É a mesma
 * regra do §1.5 do plano, agora entre duas máquinas em vez de entre dois
 * instantes.
 *
 * Os valores viram SLUG, e não número. `?e=12` seria mais curto e teria um
 * defeito fatal: o 12 é a posição no enum, e acrescentar uma estrutura no meio
 * de ids.h renumeraria tudo — todo link compartilhado passaria a abrir a
 * estrutura errada, em silêncio. O slug sai do nome (`TIPO_HASH_LINEAR` vira
 * `hash_linear`), que é estável contra reordenação e ainda é legível por quem
 * olha o link. E ele é derivado dos nomes gerados do ids.h, então não existe
 * uma segunda tabela para alguém esquecer de atualizar.
 *
 * A escrita é sempre `replaceState`. Com `pushState`, mexer no controle de
 * velocidade encheria o histórico e o botão de voltar do navegador viraria
 * desfazer — que não é o que ele promete.
 *
 * As duas metades do estado são gravadas por donos diferentes: a moldura (aba
 * e idioma) é do App, e os parâmetros são da aba que está na tela. Elas se
 * encontram aqui, e é este módulo que decide a ordem final das chaves — as da
 * aba que saiu somem, senão um link de ordenação carregaria para sempre a
 * árvore que alguém abriu antes. */

import {
  ALG_NOMES,
  DIST_NOMES,
  TIPO_NOMES,
  type Alg,
  type Dist,
  type Tipo,
} from "../core/ops";
import { idiomaAtual } from "../i18n";

export type AbaId = "estruturas" | "ordenacao" | "como";

const ABAS: readonly AbaId[] = ["estruturas", "ordenacao", "como"];

/* O link, congelado na carga do módulo.
 *
 * Ler `window.location` na hora em que cada aba monta não funciona, e a razão
 * é uma ordem que não se vê: as abas só montam depois de o wasm carregar, e
 * até lá a moldura já gravou a dela por cima — o link chega em `?e=avl&ops=…`
 * e a aba encontra `?aba=estruturas` e mais nada. Congelar aqui, antes de
 * qualquer escrita, é o que faz o estado inicial ser o que veio no endereço.
 *
 * E deixa a direção honesta: a URL é entrada uma vez, e saída dali em
 * diante. */
const INICIAL = new URLSearchParams(window.location.search);

/* ---- slugs -------------------------------------------------------------- *
 *
 * `TIPO_HASH_LINEAR` -> `hash_linear`. O prefixo some porque é redundante com
 * o nome do parâmetro, e o resto vai em caixa baixa porque um link em
 * maiúsculas parece grito.                                                  */

function paraSlug(nomes: readonly string[], valor: number): string | undefined {
  const nome = nomes[valor];
  if (nome === undefined) return undefined;

  const corte = nome.indexOf("_");
  return (corte < 0 ? nome : nome.slice(corte + 1)).toLowerCase();
}

function deSlug(nomes: readonly string[], texto: string | null): number | undefined {
  if (texto === null) return undefined;

  const procurado = texto.trim().toLowerCase();
  for (let i = 0; i < nomes.length; i++) {
    if (paraSlug(nomes, i) === procurado) return i;
  }
  return undefined;
}

function inteiro(texto: string | null, min: number, max: number): number | undefined {
  if (texto === null) return undefined;

  const n = Number(texto);
  if (!Number.isFinite(n)) return undefined;

  const truncado = Math.trunc(n);
  return truncado >= min && truncado <= max ? truncado : undefined;
}

function bandeira(texto: string | null): boolean | undefined {
  if (texto === null) return undefined;
  return texto === "1" || texto === "true";
}

/* ---- o que cabe no link ------------------------------------------------- */

export interface EstadoEstruturas {
  estrutura?: Tipo;
  capacidade?: number;
  comparar?: boolean;
  /** O script de operações, na sintaxe que a caixa da aba já aceita. */
  ops?: string;
}

export interface EstadoOrdenacao {
  alg?: Alg;
  n?: number;
  dist?: Dist;
  semente?: number;
  memoria?: number;
  corrida?: boolean;
}

export function abaDaUrl(): AbaId | undefined {
  const valor = INICIAL.get("aba");
  return ABAS.find((x) => x === valor);
}

export function estruturasDaUrl(): EstadoEstruturas {
  const p = INICIAL;

  return {
    estrutura: deSlug(TIPO_NOMES, p.get("e")) as Tipo | undefined,
    capacidade: inteiro(p.get("cap"), 1, 4096),
    comparar: bandeira(p.get("cmp")),
    ops: p.get("ops") ?? undefined,
  };
}

export function ordenacaoDaUrl(): EstadoOrdenacao {
  const p = INICIAL;

  return {
    alg: deSlug(ALG_NOMES, p.get("alg")) as Alg | undefined,
    n: inteiro(p.get("n"), 1, 4096),
    dist: deSlug(DIST_NOMES, p.get("dist")) as Dist | undefined,
    semente: inteiro(p.get("sem"), 1, 999999),
    memoria: inteiro(p.get("mem"), 2, 4096),
    corrida: bandeira(p.get("corrida")),
  };
}

/* ---- escrita ------------------------------------------------------------ */

let moldura: { aba: AbaId } = { aba: "estruturas" };
let daAba: Record<string, string> = {};

/* Vírgula e arroba são legais numa query string e a barra de endereços as
 * mostra como são; deixá-las escapadas encheria o link de %2C e tiraria dele
 * a única coisa que um link de estado tem de bom, que é dar para ler. */
function montarQuery(pares: Array<[string, string]>): string {
  return pares
    .map(([k, v]) => `${k}=${encodeURIComponent(v).replace(/%2C/g, ",").replace(/%40/g, "@")}`)
    .join("&");
}

function aplicar(): void {
  const pares: Array<[string, string]> = [
    ["aba", moldura.aba],
    ["lang", idiomaAtual()],
  ];

  for (const [k, v] of Object.entries(daAba)) {
    pares.push([k, v]);
  }

  const query = montarQuery(pares);
  const url = `${window.location.pathname}?${query}${window.location.hash}`;

  window.history.replaceState(null, "", url);
}

/** A moldura: a aba na tela. O idioma é lido do i18n na hora de gravar. */
export function gravarMoldura(aba: AbaId): void {
  moldura = { aba };
  aplicar();
}

/** Reescreve os parâmetros da aba ativa, jogando fora os da anterior. */
export function gravarEstruturas(e: EstadoEstruturas): void {
  daAba = {};
  if (e.estrutura !== undefined) {
    const slug = paraSlug(TIPO_NOMES, e.estrutura);
    if (slug !== undefined) daAba.e = slug;
  }
  if (e.capacidade !== undefined) daAba.cap = String(e.capacidade);
  if (e.comparar) daAba.cmp = "1";
  if (e.ops !== undefined && e.ops.trim().length > 0) {
    /* Numa linha só: a caixa aceita quebra de linha, e um %0A no meio do link
     * é o tipo de coisa que faz alguém achar que o link quebrou. */
    daAba.ops = e.ops.replace(/\s*\n\s*/g, ", ").trim();
  }
  aplicar();
}

export function gravarOrdenacao(o: EstadoOrdenacao): void {
  daAba = {};
  if (o.alg !== undefined) {
    const slug = paraSlug(ALG_NOMES, o.alg);
    if (slug !== undefined) daAba.alg = slug;
  }
  if (o.n !== undefined) daAba.n = String(o.n);
  if (o.dist !== undefined) {
    const slug = paraSlug(DIST_NOMES, o.dist);
    if (slug !== undefined) daAba.dist = slug;
  }
  if (o.semente !== undefined) daAba.sem = String(o.semente);
  if (o.memoria !== undefined) daAba.mem = String(o.memoria);
  if (o.corrida) daAba.corrida = "1";
  aplicar();
}

/** A aba "como funciona" não tem estado próprio: o link dela é só a moldura. */
export function limparDaAba(): void {
  daAba = {};
  aplicar();
}

/** Reescreve o link sem mudar nada — é o que o troca-idioma chama, porque o
 * idioma é lido do i18n na hora de gravar e não passa por aqui. */
export function regravar(): void {
  aplicar();
}

/** O link que está na barra agora, absoluto, para copiar. */
export function linkAtual(): string {
  return window.location.href;
}
