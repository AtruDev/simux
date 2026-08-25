/* A fatia mínima de ponta a ponta: um clique chama uma função em C compilada
 * para WebAssembly, e os eventos que ela emitiu aparecem na tela — com o
 * arquivo e a linha do .c que os produziu.
 *
 * Nada aqui é a arquitetura final. O Player e o canvas ficam fora do React;
 * esta tela existe para provar o caminho. */

import { useEffect, useState } from "react";

import { ErroDs, exec, iniciar, type Ev } from "./core/bridge";
import { ERR_CHAVES, EVKIND_NOMES, Op, SRC_NOMES, STR_CHAVES } from "./core/ops";
import {
  definirIdioma,
  idiomaAtual,
  t,
  type Chave,
  type Idioma,
} from "./i18n";

/* O C manda o id da mensagem; a frase vive no i18n. */
function mensagemDe(ev: Ev): string {
  const chave = STR_CHAVES[ev.a];
  return chave ? t(chave as Chave) : "";
}

export function App() {
  const [idioma, setIdioma] = useState<Idioma>(idiomaAtual());
  const [carregado, setCarregado] = useState(false);
  const [eventos, setEventos] = useState<Ev[]>([]);
  const [erro, setErro] = useState<string | null>(null);

  useEffect(() => {
    iniciar()
      .then(() => setCarregado(true))
      .catch((e: unknown) => setErro(String(e)));
  }, []);

  function chamarPing() {
    try {
      setEventos(exec(Op.OP_PING));
      setErro(null);
    } catch (e) {
      if (e instanceof ErroDs) {
        /* O C devolve o código; o nome e a frase moram deste lado. */
        const chave = ERR_CHAVES[e.codigo];
        setErro(chave ? t(chave as Chave) : String(e.codigo));
      } else {
        setErro(String(e));
      }
    }
  }

  function alternarIdioma() {
    const novo: Idioma = idioma === "pt" ? "en" : "pt";
    definirIdioma(novo);
    setIdioma(novo);
  }

  return (
    <main className="pagina">
      <header className="cabecalho">
        <h1>{t("app.titulo")}</h1>
        <button type="button" onClick={alternarIdioma} className="secundario">
          {t("app.trocarIdioma")}
        </button>
      </header>

      <p className="descricao">{t("app.descricao")}</p>

      <button type="button" onClick={chamarPing} disabled={!carregado}>
        {carregado ? t("app.botaoPing") : t("app.carregando")}
      </button>

      {erro !== null && (
        <p className="erro">
          {t("app.erro")}: {erro}
        </p>
      )}

      {eventos.length === 0 ? (
        <p className="vazio">{t("app.semEventos")}</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>{t("app.tabelaEvento")}</th>
              <th>{t("app.tabelaOrigem")}</th>
              <th>{t("app.tabelaLinha")}</th>
              <th>{t("app.tabelaMensagem")}</th>
            </tr>
          </thead>
          <tbody>
            {eventos.map((ev, i) => (
              <tr key={i}>
                <td className="mono">{EVKIND_NOMES[ev.kind]}</td>
                <td className="mono">{SRC_NOMES[ev.src]}</td>
                <td className="mono numero">{ev.line}</td>
                <td>{mensagemDe(ev)}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </main>
  );
}
