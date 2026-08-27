# simux

A visual simulator for data structures and algorithms. The core is written in
**plain C99** and compiled to **WebAssembly**; the web frontend animates every
operation step by step, highlighting the exact line of C that is executing.

Built as a portfolio piece and as a study aid for an undergraduate Data
Structures course.

> **Status:** Phase 4 done — two tabs, fifteen structures. Stacks, queues and
> lists in both a linked and an array implementation; sequential and binary
> search; BST and AVL; chained hashing and open addressing with three probing
> strategies. Any family can be put side by side, running the same sequence of
> operations at once. Sorting: six algorithms, a race mode, and an empirical
> mode that measures real comparison counts against the theoretical curves.

## How it works

The C core never draws anything. It runs the operation and emits a *trace* of
fixed-size events — "compared index 3 with index 4", "node 7 created", "top now
points to node 7" — into a flat buffer. JavaScript reads that buffer as an
`Int32Array` straight out of the WASM heap, with no copying and no parsing, and
replays it at whatever speed you like, with play, pause, step and scrub.

Each event carries the source file and the `__LINE__` that produced it, so the
code panel highlights real executing code rather than invented pseudocode.

## Building

Requires CMake, Ninja, a C99 compiler, the Emscripten SDK, and Node with pnpm.

```powershell
# native — tests and CLI, with sanitizers
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDS_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure

# wasm — output goes straight into web/src/wasm/
emcmake cmake -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm

# frontend
cd web; pnpm install; pnpm dev
```

## A note on language

The C source is written in Portuguese — `rebalancear`, `no->esq`,
`fila_enfileirar`, `/* caso esquerda-esquerda */` — because the code panel is
part of the product and the project serves a Portuguese-language course. The
user interface is available in both Portuguese and English.

A short glossary for readers of the C source:

| Portuguese | English | Portuguese | English |
|---|---|---|---|
| `no` | node | `esq` / `dir` | left / right |
| `fb` | balance factor | `percurso` | traversal |
| `enfileirar` | enqueue | `desenfileirar` | dequeue |
| `pilha` | stack | `fila` | queue |
| `arvore` | tree | `busca` | search |
| `ordenacao` | sorting | `troca` | swap |
| `bolha` | bubble sort | `selecao` | selection sort |
| `insercao` | insertion sort | `intercalar` | merge |
| `particionar` | partition | `medida` | measurement |

## License

MIT — see [LICENSE](LICENSE).
