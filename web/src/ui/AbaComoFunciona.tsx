/* A terceira aba: como o projeto funciona por dentro.
 *
 * O plano põe esta página acima de qualquer estrutura extra, e a razão é
 * simples: quem abre o link vê animações bonitas e não tem como saber o que
 * está por baixo. Uma pilha desenhada é uma pilha desenhada; o que distingue
 * este projeto de um visualizador em JavaScript é a fronteira, e a fronteira é
 * invisível justamente quando funciona.
 *
 * A parte que não é texto é a que vale: a tabela de eventos é gerada AGORA,
 * chamando o núcleo de verdade. Não é um exemplo escrito à mão que envelhece
 * quando a pilha mudar — se `pilha_enc.c` emitir outro evento amanhã, a
 * tabela muda junto. Os números do fim, idem: saem dos enums gerados do
 * ids.h, e não de um contador que alguém teria que lembrar de atualizar. */

import { useMemo } from "react";

import { chamar, sessaoNova, selecionarSlot, type Ev } from "../core/bridge";
import { ALG_NOMES, EVKIND_NOMES, Op, STR_CHAVES, Tipo } from "../core/ops";
import { fonteDe } from "../content/fontes";
import { t, type Chave } from "../i18n";
import { ESTRUTURAS } from "./Estruturas";

/* Quantos eventos da demonstração cabem na tabela sem ela virar rolagem. O
 * push de uma pilha encadeada emite meia dúzia; o corte existe para o dia em
 * que alguém instrumentar mais. */
const LINHAS_MAX = 12;

/* Os passos do caminho de ida e volta, na ordem em que acontecem. */
const PIPELINE = [
  "como.p1",
  "como.p2",
  "como.p3",
  "como.p4",
  "como.p5",
  "como.p6",
] as const;

/* As cinco decisões que definem o resto. Cada uma com o título e o porquê. */
const DECISOES = [
  ["como.d1", "como.d1Texto"],
  ["como.d2", "como.d2Texto"],
  ["como.d3", "como.d3Texto"],
  ["como.d4", "como.d4Texto"],
  ["como.d5", "como.d5Texto"],
] as const;

function origem(ev: Ev): string {
  const fonte = fonteDe(ev.src);
  if (!fonte) return "—";

  /* Só o nome do arquivo: o caminho inteiro empurraria a coluna e não
   * acrescenta nada aqui. */
  const nome = fonte.arquivo.split("/").pop() ?? fonte.arquivo;
  return `${nome}:${ev.line}`;
}

export function AbaComoFunciona() {
  /* Uma pilha de verdade, um push de verdade. O primeiro empilha numa pilha
   * vazia; o segundo é o interessante, porque tem a aresta para o nó que já
   * estava lá — é ele que vai para a tabela. */
  const demo = useMemo<Ev[]>(() => {
    try {
      selecionarSlot(0);
      sessaoNova(Tipo.TIPO_PILHA_ENC, 8);
      chamar(Op.OP_PUSH, 42);
      return chamar(Op.OP_PUSH, 17).eventos.slice(0, LINHAS_MAX);
    } catch {
      /* Se o núcleo não estiver carregado, a página continua legível sem a
       * tabela — ela é a prova, não o conteúdo. */
      return [];
    }
  }, []);

  /* Os mesmos eventos como o JavaScript os enxerga: seis inteiros por evento,
   * lidos direto da heap do wasm. É a frase "sem parser" virando número. */
  const cru = demo
    .slice(0, 3)
    .flatMap((e) => [e.kind, e.src, e.line, e.a, e.b, e.c])
    .join(", ");

  const numeros: Array<[Chave, number]> = [
    ["como.numEventos", EVKIND_NOMES.length - 1],
    ["como.numMensagens", STR_CHAVES.length],
    ["como.numEstruturas", ESTRUTURAS.length],
    ["como.numAlgoritmos", ALG_NOMES.length - 1],
  ];

  return (
    <div className="como">
      <article className="como-texto">
        <h2>{t("como.titulo")}</h2>
        <p className="como-tese">{t("como.tese")}</p>

        <h3>{t("como.pipelineTitulo")}</h3>
        <ol className="como-pipeline">
          {PIPELINE.map((chave, i) => (
            <li key={chave}>
              <span className="como-passo">{i + 1}</span>
              <span>{t(chave)}</span>
            </li>
          ))}
        </ol>

        <h3>{t("como.traceTitulo")}</h3>
        <p>{t("como.traceTexto")}</p>

        {demo.length > 0 ? (
          <>
            <table className="como-trace">
              <thead>
                <tr>
                  <th>{t("como.evento")}</th>
                  <th>{t("como.origem")}</th>
                  <th>a</th>
                  <th>b</th>
                  <th>c</th>
                </tr>
              </thead>
              <tbody>
                {demo.map((ev, i) => (
                  <tr key={i}>
                    <td className="mono">
                      {EVKIND_NOMES[ev.kind] ?? ev.kind}
                    </td>
                    <td className="mono como-origem">{origem(ev)}</td>
                    <td className="mono numero">{ev.a}</td>
                    <td className="mono numero">{ev.b}</td>
                    <td className="mono numero">{ev.c}</td>
                  </tr>
                ))}
              </tbody>
            </table>

            <p className="como-legenda">{t("como.brutoTexto")}</p>
            <pre className="como-bruto mono">[{cru}, …]</pre>
          </>
        ) : (
          <p className="vazio">{t("app.carregando")}</p>
        )}

        <h3>{t("como.fronteiraTitulo")}</h3>
        <p>{t("como.fronteiraTexto")}</p>
        <pre className="como-codigo mono">{`int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c);

typedef struct {
    int32_t kind;    /* ${t("como.cKind")} */
    int32_t src;     /* ${t("como.cSrc")} */
    int32_t line;    /* ${t("como.cLine")} */
    int32_t a, b, c; /* ${t("como.cAbc")} */
} ev_t;`}</pre>

        <h3>{t("como.decisoesTitulo")}</h3>
        <dl className="como-decisoes">
          {DECISOES.map(([titulo, texto]) => (
            <div key={titulo}>
              <dt>{t(titulo)}</dt>
              <dd>{t(texto)}</dd>
            </div>
          ))}
        </dl>

        <h3>{t("como.numerosTitulo")}</h3>
        <ul className="como-numeros">
          {numeros.map(([chave, valor]) => (
            <li key={chave}>
              <strong className="mono">{valor}</strong>
              <span>{t(chave)}</span>
            </li>
          ))}
        </ul>
        <p className="como-legenda">{t("como.numerosTexto")}</p>

        <p className="como-rodape">
          <a href="https://github.com/AtruDev/simux" rel="noreferrer">
            {t("como.repositorio")}
          </a>
        </p>
      </article>
    </div>
  );
}
