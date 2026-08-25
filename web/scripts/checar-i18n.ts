/* Falha se pt.ts e en.ts divergirem, ou se algum id vindo do C ficar sem
 * tradução.
 *
 * O TypeScript já cobra as chaves pelo tipo Dicionario, mas essa checagem só
 * vale enquanto alguém rodar o tsc. Este script é o que roda no CI e é o que
 * impede a versão em inglês de apodrecer. */

import { ERR_CHAVES, STR_CHAVES } from "../src/core/ops";
import en from "../src/i18n/en";
import pt from "../src/i18n/pt";

const problemas: string[] = [];

const chavesPt = Object.keys(pt).sort();
const chavesEn = Object.keys(en).sort();

for (const k of chavesPt) {
  if (!chavesEn.includes(k)) problemas.push(`falta em en.ts: ${k}`);
}
for (const k of chavesEn) {
  if (!chavesPt.includes(k)) problemas.push(`falta em pt.ts: ${k}`);
}

/* Todo STR_ e todo ERR_ do C precisa de frase nos dois idiomas. */
for (const k of [...STR_CHAVES, ...ERR_CHAVES]) {
  if (!chavesPt.includes(k)) problemas.push(`id do C sem tradução em pt.ts: ${k}`);
  if (!chavesEn.includes(k)) problemas.push(`id do C sem tradução em en.ts: ${k}`);
}

/* Uma chave existir e estar vazia nos dois idiomas é quase sempre esquecimento.
 * STR_NENHUMA é vazia de propósito: é o "nenhuma mensagem". */
for (const k of chavesPt) {
  if (k === "STR_NENHUMA") continue;
  const a = (pt as Record<string, string>)[k];
  const b = (en as Record<string, string>)[k];
  if (a === "" || b === "") problemas.push(`tradução vazia: ${k}`);
}

if (problemas.length > 0) {
  console.error("checar-i18n falhou:");
  for (const p of problemas) console.error(`  - ${p}`);
  /* Exceção não capturada já faz o node sair com status 1, o que evita
   * arrastar @types/node para dentro do projeto só por causa disto. */
  throw new Error(`checar-i18n: ${problemas.length} problema(s)`);
}

console.log(`checar-i18n: ok (${chavesPt.length} chaves nos dois idiomas)`);
