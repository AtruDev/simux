import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    // O painel de código importa os .c de core/ com ?raw, e core/ fica acima
    // da raiz do Vite. Sem isto o dev server recusa servir o arquivo.
    fs: { allow: [".."] },
  },
  // O glue do emcc é um módulo ES que localiza o .wasm por import.meta.url;
  // o Vite resolve isso sozinho, sem plugin.
});
