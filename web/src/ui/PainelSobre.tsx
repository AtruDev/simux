/* O painel que explica a estrutura que está na tela.
 *
 * Aberto por padrão, ao contrário do painel de script — este é conteúdo, e
 * conteúdo dobrado não é lido. São poucas linhas: três ou quatro de
 * complexidade e duas frases.
 *
 * No modo comparar ele mostra a estrutura SELECIONADA, e não as duas: as duas
 * já estão lado a lado no canvas, e é a diferença entre elas que o texto de
 * cada uma conta. Trocar o rádio troca o texto, que é como se lê a
 * comparação. */

import { t } from "../i18n";
import type { Estrutura } from "./Estruturas";
import { sobreDe } from "./Sobre";

interface Props {
  estrutura: Estrutura;
}

export function PainelSobre({ estrutura }: Props) {
  const sobre = sobreDe(estrutura.tipo);
  if (!sobre) return null;

  return (
    <details className="painel painel-sobre" open>
      <summary>
        <h2>
          {t("sobre.titulo")}
          <span className="arquivo">{t(estrutura.nome)}</span>
        </h2>
      </summary>

      <table className="tabela-custos">
        <thead>
          <tr>
            <th>{t("sobre.operacao")}</th>
            <th>{t("sobre.medio")}</th>
            <th>{t("sobre.pior")}</th>
          </tr>
        </thead>
        <tbody>
          {sobre.custos.map(([operacao, medio, pior]) => (
            <tr key={operacao}>
              <td>{t(operacao)}</td>
              <td className="mono">{medio}</td>
              {/* O pior caso ganha destaque quando é pior que o médio: é
                  exatamente ali que mora a lição da estrutura. */}
              <td className={medio === pior ? "mono" : "mono custo-pior"}>
                {pior}
              </td>
            </tr>
          ))}
        </tbody>
      </table>

      <p className="sobre-bloco">
        <strong>{t("sobre.quando")}</strong> {t(sobre.quando)}
      </p>
      <p className="sobre-bloco">
        <strong>{t("sobre.pega")}</strong> {t(sobre.pega)}
      </p>
    </details>
  );
}
