/* Testes do interpretador de script.
 *
 * Um parser é o tipo de código onde o teste sai mais barato que a verificação
 * à mão: cada forma que a sintaxe aceita é uma linha aqui, e as que ela tem
 * que recusar também — aceitar demais é pior que recusar, porque executa uma
 * operação que ninguém pediu. */

import { Op } from "../src/core/ops";
import { interpretar } from "../src/ui/Script";

let checagens = 0;
const falhas: string[] = [];
let caso = "(sem nome)";

function CASO(nome: string) {
  caso = nome;
}

function igual(obtido: unknown, esperado: unknown, oQue: string) {
  checagens++;
  if (obtido !== esperado) {
    falhas.push(
      `${caso}: ${oQue} — obtido ${String(obtido)}, esperado ${String(esperado)}`,
    );
  }
}

/** "PUSH 10, POP" — forma compacta de comparar o que saiu. */
function resumo(texto: string): string {
  const { passos } = interpretar(texto);
  const nome: Record<number, string> = {
    [Op.OP_PUSH]: "PUSH",
    [Op.OP_POP]: "POP",
    [Op.OP_TOPO]: "TOPO",
    [Op.OP_LIMPAR]: "LIMPAR",
  };
  return passos
    .map((p) => (p.op === Op.OP_PUSH ? `${nome[p.op]} ${p.valor}` : nome[p.op]))
    .join(", ");
}

function erros(texto: string): string {
  return interpretar(texto)
    .erros.map((e) => `${e.linha}:${e.texto}`)
    .join(", ");
}

/* ---- as formas que têm que ser aceitas --------------------------------- */

CASO("uma operação por linha");
igual(resumo("empilhar 10\nempilhar 20"), "PUSH 10, PUSH 20", "por extenso");
igual(resumo("push 10\npop"), "PUSH 10, POP", "em inglês");
igual(resumo("i 10\nr\nc\nl"), "PUSH 10, POP, TOPO, LIMPAR", "por inicial");

CASO("valor grudado no verbo");
igual(resumo("i10"), "PUSH 10", "i10");
igual(resumo("i-7"), "PUSH -7", "negativo grudado");
igual(resumo("empilhar42"), "PUSH 42", "por extenso, grudado");

CASO("separadores");
igual(resumo("i 1, i 2; i 3"), "PUSH 1, PUSH 2, PUSH 3", "vírgula e ponto e vírgula");
igual(resumo("  i 1  \n\n   r  "), "PUSH 1, POP", "espaço e linha em branco");

CASO("vários valores no mesmo verbo");
igual(resumo("inserir 1 2 3"), "PUSH 1, PUSH 2, PUSH 3", "separados por espaço");
igual(resumo("i 1, 2, 3"), "PUSH 1, PUSH 2, PUSH 3", "separados por vírgula");
igual(
  resumo("inserir 1, 2\nr"),
  "PUSH 1, PUSH 2, POP",
  "o verbo não vaza para a linha seguinte",
);
igual(erros("i 1\n2"), "2:2", "número solto em linha nova é engano");
igual(resumo("i 1, r, i 2"), "PUSH 1, POP, PUSH 2", "verbo novo no meio da linha manda");

CASO("comentários");
igual(resumo("# só comentário"), "", "linha inteira");
igual(resumo("i 5 # empilha cinco"), "PUSH 5", "no fim da linha");

CASO("família trocada é aceita de propósito");
igual(resumo("enfileirar 3\ndesenfileirar"), "PUSH 3, POP", "verbo de fila");

CASO("o valor é truncado para inteiro, que é o elem_t");
igual(resumo("i 3.9"), "PUSH 3", "3.9 vira 3");
igual(resumo("i -3.9"), "PUSH -3", "trunca em direção ao zero");

/* ---- as que têm que ser recusadas -------------------------------------- */

CASO("verbo desconhecido");
igual(erros("rotacionar 3"), "1:rotacionar 3", "não inventa operação");
igual(resumo("rotacionar 3"), "", "e não executa nada");

CASO("inserir sem valor");
igual(erros("empilhar"), "1:empilhar", "recusa");

CASO("valor onde não cabe");
igual(erros("desempilhar 3"), "1:desempilhar 3", "não é 'três vezes'");
igual(erros("pop 2, r"), "1:pop 2", "e recusa só o comando errado");

/* "consultar topo" é o rótulo do próprio botão da interface, e era recusado
 * pela primeira versão da regra acima. Prosa depois do verbo passa; número,
 * não. */
CASO("prosa depois de verbo sem valor");
igual(resumo("consultar topo"), "TOPO", "consultar topo");
igual(resumo("consultar frente"), "TOPO", "consultar frente");
igual(resumo("remover do topo"), "POP", "remover do topo");

CASO("valor que não é número");
igual(erros("i abc"), "1:i abc", "recusa");
igual(erros("i 1 abc"), "1:i 1 abc", "recusa a lista inteira");
igual(resumo("i 1 abc"), "", "e não insere o 1 que estava certo");

/* A numeração de linha era o defeito da primeira versão: quebrando o texto
 * todo num split só, "i 1, i 2" gastava dois números e o erro apontava para
 * uma linha que não existia no que a pessoa escreveu. */
CASO("a linha do erro é a linha de verdade");
igual(erros("i 1, i 2\nxyz"), "2:xyz", "vírgula não adianta o contador");
igual(erros("i 1\n\n\nxyz"), "4:xyz", "linha em branco conta");
igual(erros("# nota\nxyz"), "2:xyz", "comentário conta");

/* ------------------------------------------------------------------------ */

if (falhas.length > 0) {
  console.error("testar-script falhou:");
  for (const f of falhas) console.error(`  - ${f}`);
  throw new Error(`testar-script: ${falhas.length} falha(s)`);
}

console.log(`testar-script: ok (${checagens} checagens)`);
