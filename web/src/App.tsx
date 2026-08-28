/* A moldura: o cabeçalho, o idioma e as três abas.
 *
 * Ela nasceu quando a aba de ordenação chegou. Antes, App.tsx ERA a tela das
 * estruturas; agora ele é só o que as duas dividem, e cada aba mora no seu
 * arquivo com o seu Player.
 *
 * Um Player por aba, e não um compartilhado: as linhas do tempo são de coisas
 * diferentes, e trocar de aba não pode jogar fora a sequência de operações que
 * estava montada na outra. Elas são montadas e desmontadas com a aba, o que
 * também para o laço de animação da que não está na tela.
 *
 * O núcleo do wasm é único, isso sim — ele carrega aqui, uma vez, e as duas
 * abas usam os mesmos slots de sessão. A que entra abre a sua sessão e
 * descarta a da outra, que é o comportamento certo: o C guarda a estrutura, e
 * a estrutura da aba que saiu não está mais na tela. */

import { useEffect, useState } from "react";

import { iniciar } from "./core/bridge";
import { definirIdioma, idiomaAtual, t, type Chave, type Idioma } from "./i18n";
import { AbaEstruturas } from "./ui/AbaEstruturas";
import { AbaComoFunciona } from "./ui/AbaComoFunciona";
import { AbaOrdenacao } from "./ui/AbaOrdenacao";

type Aba = "estruturas" | "ordenacao" | "como";

const ABAS: Array<{ id: Aba; nome: Chave }> = [
  { id: "estruturas", nome: "aba.estruturas" },
  { id: "ordenacao", nome: "aba.ordenacao" },
  /* A terceira não é uma aba de simulação: é a explicação do que as outras
   * duas estão fazendo por baixo. Fica por último porque é onde se vai depois
   * de ver a coisa funcionar, e não antes. */
  { id: "como", nome: "aba.comoFunciona" },
];

export function App() {
  const [idioma, setIdioma] = useState<Idioma>(idiomaAtual());
  const [aba, setAba] = useState<Aba>("estruturas");
  const [pronto, setPronto] = useState(false);
  const [erro, setErro] = useState<string | null>(null);

  /* O módulo carrega uma vez, aqui. Cada aba montar o seu próprio carregamento
   * daria dois `criarSimux` — o bridge é idempotente, mas a aba começaria a
   * desenhar antes de o núcleo existir. */
  useEffect(() => {
    let vivo = true;
    iniciar()
      .then(() => vivo && setPronto(true))
      .catch((e: unknown) => vivo && setErro(String(e)));
    return () => {
      vivo = false;
    };
  }, []);

  function alternarIdioma() {
    const novo: Idioma = idioma === "pt" ? "en" : "pt";
    definirIdioma(novo);
    setIdioma(novo);
  }

  return (
    <div className="app">
      <header className="cabecalho">
        <h1>{t("app.titulo")}</h1>
        <p className="descricao">{t("app.descricao")}</p>

        <nav className="abas" aria-label={t("app.titulo")}>
          {ABAS.map((x) => (
            <button
              key={x.id}
              type="button"
              className={aba === x.id ? "aba ativa" : "aba"}
              aria-current={aba === x.id ? "page" : undefined}
              onClick={() => setAba(x.id)}
            >
              {t(x.nome)}
            </button>
          ))}
        </nav>

        <button type="button" onClick={alternarIdioma} className="secundario">
          {t("app.trocarIdioma")}
        </button>
      </header>

      {erro !== null && (
        <p className="erro">
          {t("app.erro")}: {erro}
        </p>
      )}

      {!pronto ? (
        <p className="vazio">{t("app.carregando")}</p>
      ) : aba === "estruturas" ? (
        <AbaEstruturas />
      ) : aba === "ordenacao" ? (
        <AbaOrdenacao />
      ) : (
        <AbaComoFunciona />
      )}
    </div>
  );
}
