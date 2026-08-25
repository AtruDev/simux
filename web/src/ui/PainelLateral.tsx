/* Métricas e log. Os dois leem o mesmo modelo que o canvas desenha, então não
 * existe a possibilidade de o número discordar do desenho. */

import { EvKind, Cnt, Ptr, STR_CHAVES } from "../core/ops";
import type { Ev } from "../core/bridge";
import { contador, alvoDe, type Modelo } from "../model/modelo";
import { t, type Chave } from "../i18n";

interface PropsMetricas {
  modelo: Modelo;
  i: number;
  total: number;
}

export function PainelMetricas({ modelo, i, total }: PropsMetricas) {
  const linhas: Array<[string, string]> = [
    [t("metrica.tamanho"), String(contador(modelo, Cnt.CNT_TAMANHO))],
    [t("metrica.alocacoes"), String(contador(modelo, Cnt.CNT_ALOCACOES))],
    [t("metrica.nos"), String(modelo.nos.size)],
    [t("metrica.eventos"), `${i} / ${total}`],
  ];

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
};

/** Descreve um evento em português ou inglês, a partir do id — nunca de texto
 * vindo do C, que não devolve texto nenhum. */
export function descrever(ev: Ev): string {
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
    case EvKind.EV_PTR_SET:
      return ev.a === Ptr.PTR_TOPO
        ? `${t("log.ponteiro")} ${no(ev.b)}`
        : `ptr ${ev.a} → ${no(ev.b)}`;
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

export function PainelLog({ eventos }: { eventos: Ev[] }) {
  return (
    <section className="painel painel-log">
      <h2>{t("painel.log")}</h2>
      {eventos.length === 0 ? (
        <p className="vazio">{t("log.vazio")}</p>
      ) : (
        <ol className="log">
          {eventos.map((ev, k) => (
            <li key={`${k}-${ev.line}`} className={k === 0 ? "atual" : ""}>
              {descrever(ev)}
            </li>
          ))}
        </ol>
      )}
    </section>
  );
}

/** Só para o painel de métricas saber se o topo está nulo. */
export function topoNulo(m: Modelo): boolean {
  return alvoDe(m, Ptr.PTR_TOPO) === 0;
}
