#!/usr/bin/env python3
"""Gera web/src/core/ops.ts a partir dos headers de enums do core.

Os mesmos enums existem nos dois lados da fronteira. Mantê-los na mão
dessincroniza mais cedo ou mais tarde, e o sintoma é mudo: a animação passa a
fazer a coisa errada e nada quebra. Este script torna o .h a única fonte.

Formato aceito, e é o que ids.h documenta:

    typedef enum {
        NOME_UM,
        NOME_DOIS,
    } nome_do_tipo;

Um identificador por entrada, sem inicializador explícito — a ordem é que
define o valor, nos dois lados. Um `=` no meio da lista é erro, porque
quebraria justamente a correspondência que o script existe para garantir.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# typedef enum { ... } nome;
ENUM = re.compile(r"typedef\s+enum\s*\{(?P<corpo>[^{}]*)\}\s*(?P<nome>\w+)\s*;",
                  re.DOTALL)

COMENTARIO_BLOCO = re.compile(r"/\*.*?\*/", re.DOTALL)
COMENTARIO_LINHA = re.compile(r"//[^\n]*")

# nome do typedef em C -> nome do enum em TypeScript
RENOMES = {
    "ev_kind": "EvKind",
    "ds_src": "Src",
    "ds_str": "Str",
    "ds_ptr": "Ptr",
    "ds_cnt": "Cnt",
    "ds_tag": "Tag",
    "ds_op": "Op",
    "ds_tipo": "Tipo",
    "ds_status": "Status",
}


class ErroDeFormato(Exception):
    pass


def sem_comentarios(texto: str) -> str:
    texto = COMENTARIO_BLOCO.sub(" ", texto)
    return COMENTARIO_LINHA.sub(" ", texto)


def ler_enums(caminho: Path) -> list[tuple[str, list[str]]]:
    fonte = sem_comentarios(caminho.read_text(encoding="utf-8"))
    achados: list[tuple[str, list[str]]] = []

    for m in ENUM.finditer(fonte):
        nome = m.group("nome")
        membros: list[str] = []

        for bruto in m.group("corpo").split(","):
            entrada = bruto.strip()
            if not entrada:
                continue
            if "=" in entrada:
                raise ErroDeFormato(
                    f"{caminho.name}: '{entrada}' em '{nome}' tem valor "
                    f"explícito. A ordem é que define o valor nos dois lados; "
                    f"um inicializador aqui desalinha o TypeScript em silêncio."
                )
            if not entrada.isidentifier():
                raise ErroDeFormato(
                    f"{caminho.name}: '{entrada}' em '{nome}' não é um "
                    f"identificador simples."
                )
            membros.append(entrada)

        if not membros:
            raise ErroDeFormato(f"{caminho.name}: enum '{nome}' está vazio.")

        vistos = {x for x in membros if membros.count(x) > 1}
        if vistos:
            raise ErroDeFormato(
                f"{caminho.name}: '{nome}' repete {sorted(vistos)}."
            )

        achados.append((nome, membros))

    return achados


def nome_ts(nome_c: str) -> str:
    if nome_c in RENOMES:
        return RENOMES[nome_c]
    limpo = nome_c[3:] if nome_c.startswith("ds_") else nome_c
    return "".join(parte.capitalize() for parte in limpo.split("_"))


def bloco_enum(nome_c: str, membros: list[str]) -> str:
    alvo = nome_ts(nome_c)
    linhas = [f"/** `{nome_c}` */", f"export enum {alvo} {{"]
    linhas += [f"  {m} = {i}," for i, m in enumerate(membros)]
    linhas.append("}")
    linhas.append("")
    linhas.append(f"/** Nome de cada valor de {alvo}, indexado pelo valor. */")
    linhas.append(f"export const {alvo.upper()}_NOMES: readonly string[] = [")
    linhas += [f"  {m!r}," .replace("'", '"') for m in membros]
    linhas.append("];")
    return "\n".join(linhas)


def bloco_chaves(alvo: str, membros: list[str], comentario: str) -> str:
    uteis = [m for m in membros if not m.endswith("_COUNT")]
    linhas = [f"/** {comentario} */", f"export const {alvo} = ["]
    linhas += [f'  "{m}",' for m in uteis]
    linhas.append("] as const;")
    linhas.append("")
    singular = alvo.rstrip("S").title().replace("_", "")
    linhas.append(f"export type {singular} = (typeof {alvo})[number];")
    return "\n".join(linhas)


def gerar(entradas: list[tuple[str, list[str]]], origens: list[Path]) -> str:
    fontes = ", ".join(p.name for p in origens)
    partes = [
        "/* ARQUIVO GERADO — NÃO EDITE.",
        " *",
        f" * Produzido por tools/gen_enums.py a partir de {fontes}.",
        " * Para mudar qualquer coisa aqui, mude o header e reconstrua.",
        " */",
        "",
        "/* eslint-disable */",
        "",
    ]

    por_nome = dict(entradas)
    partes += [bloco_enum(nome, membros) + "\n" for nome, membros in entradas]

    if "ds_str" in por_nome:
        partes.append(bloco_chaves(
            "STR_CHAVES", por_nome["ds_str"],
            "Toda mensagem que o C pode emitir. Cada uma precisa de tradução "
            "em pt.ts e en.ts — é isto que faz o build reclamar quando falta.",
        ) + "\n")

    if "ds_status" in por_nome:
        partes.append(bloco_chaves(
            "ERR_CHAVES", por_nome["ds_status"],
            "Todo código de erro que o core devolve, para o mesmo fim.",
        ) + "\n")

    partes.append("/** Inteiros por evento no buffer de trace. */")
    partes.append("export const EV_CAMPOS = 6;")

    return "\n".join(partes) + "\n"


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("headers", nargs="+", type=Path)
    ap.add_argument("--saida", required=True, type=Path)
    args = ap.parse_args(argv)

    entradas: list[tuple[str, list[str]]] = []
    try:
        for h in args.headers:
            entradas += ler_enums(h)
    except (ErroDeFormato, OSError) as e:
        print(f"gen_enums: {e}", file=sys.stderr)
        return 1

    if not entradas:
        print("gen_enums: nenhum enum encontrado", file=sys.stderr)
        return 1

    texto = gerar(entradas, args.headers)

    # Não reescrever quando nada mudou: evita disparar rebuild do Vite à toa.
    if args.saida.exists() and args.saida.read_text(encoding="utf-8") == texto:
        return 0

    args.saida.parent.mkdir(parents=True, exist_ok=True)
    args.saida.write_text(texto, encoding="utf-8", newline="\n")
    print(f"gen_enums: {args.saida} ({len(entradas)} enums)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
