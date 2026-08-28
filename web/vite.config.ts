import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],

  /* Caminhos relativos no bundle, e não absolutos.
   *
   * O GitHub Pages serve o projeto em /simux/, e não na raiz do domínio: com
   * o base padrão ("/"), o index.html pediria /assets/... e a página abriria
   * em branco, sem erro visível no console além do 404. Escrever "/simux/"
   * aqui resolveria também, mas amarraria o bundle ao nome do repositório —
   * com "./" o mesmo dist funciona na raiz, num subdiretório qualquer e num
   * domínio próprio, que é justamente a decisão que a §14.1 do plano deixou
   * em aberto. */
  base: "./",
  server: {
    port: 5173,
    // O painel de código importa os .c de core/ com ?raw, e core/ fica acima
    // da raiz do Vite. Sem isto o dev server recusa servir o arquivo.
    fs: { allow: [".."] },
  },
  // O glue do emcc é um módulo ES que localiza o .wasm por import.meta.url;
  // o Vite resolve isso sozinho, sem plugin.
});
