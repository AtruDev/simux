/* Tipos do glue gerado pelo emcc.
 *
 * O simux.js e o simux.wasm saem do build e estão no .gitignore, então a
 * declaração de tipos precisa morar num arquivo versionado. Se um símbolo for
 * acrescentado aqui, ele também tem que entrar em EXPORTED_FUNCTIONS no
 * CMakeLists — senão existe em TypeScript e não em tempo de execução. */

declare module "*/wasm/simux.js" {
  export interface ModuloSimux {
    _ds_sessao_nova(tipo: number, capacidade: number): number;
    _ds_sessao_fim(): void;
    _ds_tipo_sessao(): number;
    _ds_capacidade(): number;
    _ds_call(op: number, a: number, b: number, c: number): number;
    _ds_erro(): number;
    _ds_trace_ptr(): number;
    _ds_trace_len(): number;
    _ds_trace_truncado(): number;

    /* Estas views são trocadas pelo próprio glue quando a heap cresce.
     * Leia-as sempre pelo módulo, nunca guarde uma cópia. */
    readonly HEAP32: Int32Array;
    readonly HEAPU8: Uint8Array;
  }

  const criarSimux: (opcoes?: Record<string, unknown>) => Promise<ModuloSimux>;
  export default criarSimux;
}
