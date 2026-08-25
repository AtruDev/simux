# Simux

Simulador visual de estruturas de dados. Núcleo em **C99 puro** compilado para **WebAssembly**;
frontend web anima cada operação passo a passo, destacando a linha do `.c` que está executando.

Projeto de portfólio + apoio à disciplina de Estrutura de Dados. Plano completo em `docs/plano.md`
— leia-o antes de decisões de arquitetura. Este arquivo carrega só as regras que não podem ser quebradas.

---

## Invariantes

Violar qualquer uma destas quebra a arquitetura. Se algo parecer exigir isso, **pare e pergunte**.

1. **O C nunca desenha.** Ele executa a operação e emite eventos. Nenhuma coordenada, cor, largura
   ou string de layout aparece em `core/`.
2. **O C nunca devolve texto.** `EV_MSG` carrega um id (`STR_*`); a tradução vive em `web/src/i18n/`.
   Não existe `ds_strings()`, não existe arena de strings no núcleo.
3. **Zero parser de JSON em C.** A fronteira é `ds_call(int op, int a, int b, int c)` e um buffer
   binário de eventos lido como `Int32Array`. Se um dado não couber em inteiros, ele passa pelo
   buffer de entrada (`ds_buffer()`), nunca por string.
4. **`core/` não conhece Emscripten.** Só `core/api/api.c` tem `#ifdef __EMSCRIPTEN__`.
   O mesmo código compila nativo (testes, CLI, ASAN) e para wasm.
5. **Voltar no tempo = reexecutar.** Para ir ao passo `k`, zera o modelo e aplica os eventos `0..k`.
   Nunca implemente a operação inversa de um evento.
6. **Nenhum texto literal voltado ao usuário em componente.** Tudo passa por `t('chave')`, desde
   o primeiro commit. Vale também para rótulos desenhados no `<canvas>`.
7. **O tipo de dado é `int`.** Existe `typedef int elem_t;` em `core/include/ds/tipos.h` — use-o,
   para que uma eventual generalização futura seja uma troca em um lugar só.

---

## Layout

```
core/                C99, zero dependências
  include/ds/        headers públicos + ids.h (enums compartilhados) + tipos.h
  trace/             trace.c (buffer de eventos) · idmap.c (ponteiro → id estável)
  ds/                linear.h (vtable TAD_Linear) · pilha_enc.c · pilha_vet.c · fila_*.c · abb.c · avl.c ...
  sort/              bolha.c · insercao.c · quick.c · merge.c ...
  api/               api.c — único arquivo que sabe da existência do WASM
cli/                 binário de terminal (debug + trabalhos da matéria)
tests/               runner próprio + invariantes + fuzz diferencial
tools/gen_enums.py   ids.h → web/src/core/ops.ts (roda no build)
web/                 Vite + TypeScript + React (chrome) + Canvas 2D (visualização)
docs/plano.md        o plano completo
```

## Comandos

O ambiente de desenvolvimento é **Windows + PowerShell + MinGW-w64**. Gere scripts `.ps1`,
não `.sh`. Caminhos com `\` ou `/` funcionam no CMake; prefira `/`.

```powershell
# nativo — testes e CLI, com sanitizers
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDS_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure

# wasm — saída direto em web/src/wasm/
emcmake cmake -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm

# frontend
cd web; pnpm install; pnpm dev
```

Rode `ctest` antes de considerar qualquer tarefa pronta.

---

## Convenções do C

- **Identificadores e comentários em português.** `rebalancear`, `no->esq`, `fila_enfileirar`,
  `/* caso esquerda-esquerda */`. Identificadores **sem acento** (C não aceita); comentários com
  acento, arquivos em UTF-8.
- C99. Sem VLA, sem extensões de compilador. `-Wall -Wextra -Werror`.
- Toda função pública devolve `int` de status (`OK`, `ERR_*` em `erros.h`); saída por ponteiro.
- Uma estrutura por par `.c`/`.h`. Implementações alternativas do mesmo TAD ganham sufixo
  (`_enc` para encadeada, `_vet` para vetor) e são despachadas pelo vtable `TAD_Linear`.
- Instrumentação: `#define TR_SRC SRC_<ARQUIVO>` no topo, e `TR(EV_..., .a = ..., .b = ...)`
  nos pontos de interesse. A macro grava `__LINE__` sozinha.
- **O algoritmo tem que continuar legível.** Se as chamadas `TR` estiverem atrapalhando a leitura,
  o problema é o vocabulário de eventos, não o algoritmo.

## Vocabulário de eventos

Pequeno de propósito. **Não crie um evento por estrutura** — reaproveite. `EV_PTR_SET` serve tanto
para "topo aponta para o nó 7" quanto para "início vale 5" (índice).

```
genéricos     EV_MSG(a=STR_*) · EV_COUNT(a=contador, b=delta) · EV_PHASE(a=fase)
vetor         EV_ARR_INIT(n) · EV_ARR_READ(i) · EV_ARR_COMPARE(i,j) · EV_ARR_SWAP(i,j)
              EV_ARR_WRITE(i,v) · EV_ARR_RANGE(lo,hi) · EV_ARR_MARK(i,tag)
              EV_AUX_INIT(n) · EV_AUX_WRITE(i,v)
nós           EV_NODE_NEW(id,v) · EV_NODE_FREE(id) · EV_NODE_SET(id,slot,v)
              EV_EDGE_SET(de,slot,para) · EV_PTR_SET(ptr,alvo) · EV_VISIT(id) · EV_UNVISIT(id)
disco         EV_DISK_READ(pag) · EV_DISK_WRITE(pag)
```

`id_de(NULL)` devolve `0`, e `0` significa NULL no frontend.

## Convenções do frontend

- **Estado da animação fora do React.** Um `Player` com `requestAnimationFrame` muta o modelo e
  desenha no canvas. React só recebe o que aparece nos painéis, com throttle (~15 Hz, não 60).
- **Recrie a view `Int32Array` depois de toda chamada ao WASM.** Com `ALLOW_MEMORY_GROWTH`,
  qualquer `malloc` que cresça a heap desanexa as views antigas. Guardar a view em variável de
  módulo é bug garantido, e só aparece quando o dataset cresce.
- Layout de árvore: **Reingold–Tilford**. Nunca `x = pai.x ± largura / 2^nivel`.
- Tween: cada nó tem posição atual e alvo; `atual += (alvo - atual) * 0.2` por frame.
  É isso que faz rotação de AVL e split de árvore B ficarem bons, sem lógica de animação específica.
- `devicePixelRatio` no canvas, `prefers-reduced-motion` respeitado.

## Tokens de design

Definidos em `web/src/styles/tokens.css`; o renderizador os lê com `getComputedStyle`, para não
existir uma segunda paleta no código.

```
superfícies  --bg-0 #0b0c10 · --bg-1 #111317 · --bg-2 #191c21 · --bg-3 #21252b · --canvas #07080b
traço/tinta  --line #272a30 · --line-2 #3e4148 · --fg #f2f3f6 · --fg-2 #a6a9b0 · --fg-3 #71747c
acento       --accent #6a99ff · --accent-hi #95bcff · --accent-dim #314c8c
estados      --st-compare #b98a00 · --st-swap #ae4440 · --st-done #39ac6d
             --st-pivot #885cb5 · --st-aux #0085ca
algoritmo    --alg-1 #5889e6 · --alg-2 #c97500 · --alg-3 #00a890
tipografia   Space Grotesk (display) · Instrument Sans (interface) · JetBrains Mono (código, números)
```

Três regras de cor:

1. O acento é da **interface** (foco, seleção, transporte). Nunca preenche célula nem nó —
   exceto o cursor, que é justamente "onde a interface está olhando".
2. Identidade de algoritmo é **linha fina**; estado é **preenchimento**. Na aba de ordenação o
   quicksort é uma régua no cabeçalho e uma linha no gráfico, nunca a cor de uma barra.
3. Nenhum estado é distinguido só por matiz — cada um tem tratamento de traço próprio (sólido para
   o que o algoritmo toca, tracejado para auxiliar/temporário) e a legenda fica sempre visível.
   A paleta foi validada para protanopia e deuteranopia; **não troque essas cores sem revalidar**.

---

## Não faça

- Não escreva código de desenho, layout ou cor em `core/`.
- Não adicione dependência ao `core/`. Zero, inclusive para testes.
- Não invente evento novo antes de tentar compor os existentes.
- Não edite `web/src/core/ops.ts` à mão — ele é gerado de `ids.h` por `tools/gen_enums.py`.
- Não use `rand()`; o gerador pseudoaleatório é próprio e com semente, para as cenas serem
  reproduzíveis em qualquer máquina.
- Não faça commit sem `ctest` passando.
