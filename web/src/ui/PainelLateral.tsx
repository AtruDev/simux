/* Métricas e log. Os dois leem o mesmo modelo que o canvas desenha, então não
 * existe a possibilidade de o número discordar do desenho. */

import { EvKind, Cnt, Ptr, Tag, STR_CHAVES } from "../core/ops";
import type { Ev } from "../core/bridge";
import { contador, type Modelo } from "../model/modelo";
import { t, type Chave } from "../i18n";
import type { Mundo } from "./Estruturas";

interface PropsMetricas {
  modelo: Modelo;
  i: number;
  total: number;
}

export function PainelMetricas({ modelo, i, total }: PropsMetricas) {
  const tamanho = contador(modelo, Cnt.CNT_TAMANHO);

  /* As duas implementações contam histórias diferentes de propósito: a
   * encadeada mostra alocações, a com vetor mostra escritas e ocupação. É
   * esse contraste que justifica ver as duas. */
  const linhas: Array<[string, string]> = [
    [t("metrica.tamanho"), String(tamanho)],
  ];

  if (modelo.vetor) {
    const cap = modelo.vetor.capacidade;
    linhas.push([t("metrica.capacidade"), String(cap)]);
    linhas.push([t("metrica.escritas"), String(contador(modelo, Cnt.CNT_ESCRITAS))]);
    linhas.push([
      t("metrica.ocupacao"),
      cap > 0 ? `${Math.round((tamanho / cap) * 100)}%` : "—",
    ]);
  } else {
    linhas.push([t("metrica.alocacoes"), String(contador(modelo, Cnt.CNT_ALOCACOES))]);
    linhas.push([t("metrica.nos"), String(modelo.nos.size)]);
  }

  linhas.push([t("metrica.eventos"), `${i} / ${total}`]);

  return (
    <section className="painel">
      <h2>{t("painel.metricas")}</h2>
      <dl className="metricas">
        {linhas.map(([rotulo, valor]) => (
          <div key={rotulo}>
            <dt>{rotulo}</dt>
            <dd className="mono numero">{valor}</dd>
          </div>
        ))}
      </dl>
    </section>
  );
}

/** Um id de nó como o desenho o mostra, ou NULO quando o C mandou 0. */
function no(id: number): string {
  return id === 0 ? t("log.nulo") : `#${id}`;
}

const NOME_CONTADOR: Partial<Record<number, Chave>> = {
  [Cnt.CNT_TAMANHO]: "metrica.tamanho",
  [Cnt.CNT_ALOCACOES]: "metrica.alocacoes",
  [Cnt.CNT_ESCRITAS]: "metrica.escritas",
};

const NOME_PONTEIRO: Partial<Record<number, Chave>> = {
  [Ptr.PTR_TOPO]: "log.ponteiro",
  [Ptr.PTR_FRENTE]: "log.frente",
  [Ptr.PTR_FIM]: "log.fim",
};

/* O mesmo EV_PTR_SET carrega coisas diferentes conforme o mundo, e sem saber
 * qual é não dá para traduzir: no vetor, 0 é um índice legítimo; no
 * encadeado, 0 é NULL. Foi por pouco que isto não virou um log mentindo
 * "NULO" para a célula zero. */
function alvoPonteiro(valor: number, mundo: Mundo): string {
  if (mundo === "vetor") {
    return valor < 0 ? "—" : `[${valor}]`;
  }
  return valor === 0 ? t("log.nulo") : `#${valor}`;
}

/** Descreve um evento em português ou inglês, a partir do id — nunca de texto
 * vindo do C, que não devolve texto nenhum. */
export function descrever(ev: Ev, mundo: Mundo): string {
  switch (ev.kind) {
    case EvKind.EV_MSG: {
      const chave = STR_CHAVES[ev.a];
      return chave ? t(chave as Chave) : "";
    }
    case EvKind.EV_NODE_NEW:
      return `${t("log.noCriado")} ${no(ev.a)} = ${ev.b}`;
    case EvKind.EV_NODE_FREE:
      return `${t("log.noLiberado")} ${no(ev.a)}`;
    case EvKind.EV_EDGE_SET:
      return `${no(ev.a)} ${t("log.aresta")} ${no(ev.c)}`;
    case EvKind.EV_PTR_SET: {
      const nome = NOME_PONTEIRO[ev.a];
      const rotulo = nome ? t(nome) : `ptr ${ev.a} →`;
      return `${rotulo} ${alvoPonteiro(ev.b, mundo)}`;
    }

    case EvKind.EV_ARR_INIT:
      return `${t("log.vetorInicia")} ${ev.a}`;

    case EvKind.EV_ARR_WRITE:
      return `${t("log.escreve")} [${ev.a}] = ${ev.b}`;

    case EvKind.EV_ARR_READ:
      return `${t("log.le")} [${ev.a}]`;

    case EvKind.EV_ARR_MARK:
      return ev.b === Tag.TAG_LIVRE
        ? `${t("log.marcaLivre")} [${ev.a}]`
        : `mark [${ev.a}] = ${ev.b}`;
    case EvKind.EV_VISIT:
      return `${t("log.visita")} ${no(ev.a)}`;
    case EvKind.EV_UNVISIT:
      return `${t("log.saiVisita")} ${no(ev.a)}`;
    case EvKind.EV_COUNT: {
      const nome = NOME_CONTADOR[ev.a];
      const rotulo = nome ? t(nome) : t("log.contador");
      return `${rotulo} ${ev.b >= 0 ? "+" : ""}${ev.b}`;
    }
    default:
      return EvKind[ev.kind] ?? String(ev.kind);
  }
}

export function PainelLog({
  eventos,
  mundo,
}: {
  eventos: Ev[];
  mundo: Mundo;
}) {
  return (
    <section className="painel painel-log">
      <h2>{t("painel.log")}</h2>
      {eventos.length === 0 ? (
        <p className="vazio">{t("log.vazio")}</p>
      ) : (
        <ol className="log">
          {eventos.map((ev, k) => (
            <li key={`${k}-${ev.line}`} className={k === 0 ? "atual" : ""}>
              {descrever(ev, mundo)}
            </li>
          ))}
        </ol>
      )}
    </section>
  );
}
