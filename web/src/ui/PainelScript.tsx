/* web/src/ui/PainelScript.tsx — a caixa onde se cola um exercício.
 *
 * Fica fechado por padrão: quem chegou para ver uma pilha animar não deveria
 * encontrar uma caixa de texto vazia na frente. Quem veio reproduzir a lista
 * da matéria abre uma vez e usa o resto da sessão. */

import { useState } from "react";

import { t } from "../i18n";
import { interpretar, type Erro, type Passo } from "./Script";

interface Props {
  desativado: boolean;
  aoRodar: (passos: Passo[]) => void;
}

export function PainelScript({ desativado, aoRodar }: Props) {
  const [texto, setTexto] = useState("");
  const [erros, setErros] = useState<Erro[]>([]);

  function rodar() {
    const { passos, erros: achados } = interpretar(texto);
    setErros(achados);

    /* Nada é executado enquanto houver linha não entendida. Rodar metade de um
     * script digitado errado deixa a estrutura num estado que não corresponde
     * nem ao que se escreveu nem ao que se queria — e aí não dá para saber se
     * o erro foi da pessoa ou do simulador. */
    if (achados.length === 0 && passos.length > 0) {
      aoRodar(passos);
    }
  }

  return (
    <details className="painel painel-script">
      <summary>
        <h2>{t("painel.script")}</h2>
      </summary>

      <textarea
        className="mono script"
        rows={5}
        spellCheck={false}
        value={texto}
        placeholder={t("script.exemplo")}
        onChange={(e) => setTexto(e.target.value)}
        onKeyDown={(e) => {
          /* Ctrl+Enter roda; Enter sozinho continua quebrando linha, que é o
           * que se espera de uma caixa de várias linhas. */
          if (e.key === "Enter" && (e.ctrlKey || e.metaKey)) {
            e.preventDefault();
            rodar();
          }
        }}
      />

      <p className="ajuda">{t("script.ajuda")}</p>

      <button type="button" disabled={desativado} onClick={rodar}>
        {t("script.rodar")}
      </button>

      {erros.length > 0 && (
        <ul className="erro erros-script">
          {erros.map((e, i) => (
            <li key={i}>
              {t("script.linha")} {e.linha}: <code>{e.texto}</code>
            </li>
          ))}
        </ul>
      )}
    </details>
  );
}
