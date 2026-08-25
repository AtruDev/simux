/* O painel de código-fonte, com destaque na linha que está executando.
 *
 * O texto vem do .c importado cru; a linha vem do __LINE__ que a macro TR
 * gravou dentro do evento. Os dois lados nunca podem discordar, porque são
 * literalmente o mesmo arquivo. */

import { useEffect, useRef } from "react";

import { fonteDe } from "../content/fontes";
import { t } from "../i18n";

interface Props {
  src: number;
  linha: number;
}

/** Quantas linhas mostrar em volta da que está executando. */
const CONTEXTO = 14;

export function PainelCodigo({ src, linha }: Props) {
  const fonte = fonteDe(src);
  const refAtiva = useRef<HTMLDivElement>(null);

  useEffect(() => {
    refAtiva.current?.scrollIntoView({ block: "center", behavior: "smooth" });
  }, [src, linha]);

  if (!fonte) {
    return (
      <section className="painel painel-codigo">
        <h2>{t("painel.codigo")}</h2>
        <p className="vazio">{t("log.vazio")}</p>
      </section>
    );
  }

  const inicio = Math.max(0, linha - 1 - CONTEXTO);
  const fim = Math.min(fonte.linhas.length, linha - 1 + CONTEXTO);
  const fatia = fonte.linhas.slice(inicio, fim);

  return (
    <section className="painel painel-codigo">
      <h2>
        {t("painel.codigo")}
        <span className="arquivo">{fonte.arquivo}</span>
      </h2>
      <div className="codigo">
        {fatia.map((texto, k) => {
          const numero = inicio + k + 1;
          const ativa = numero === linha;
          return (
            <div
              key={numero}
              ref={ativa ? refAtiva : undefined}
              className={ativa ? "linha ativa" : "linha"}
            >
              <span className="numero">{numero}</span>
              <code>{texto || " "}</code>
            </div>
          );
        })}
      </div>
    </section>
  );
}
