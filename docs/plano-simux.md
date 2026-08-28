---
tags: [projeto, estrutura-de-dados, c, wasm, portfolio]
status: planejamento
criado: 2026-08-25
---

# Simux — Plano de Desenvolvimento

Core em **C puro** compilado para **WebAssembly**, frontend web que anima passo a passo.
Objetivo: portfólio (arquitetura limpa, testes, CI, deploy) + apoio à disciplina de Estrutura de Dados.

Repositório: `simux` · Pages: `[SEU-USUARIO].github.io/simux`

---

## 1. Decisões-chave (leia isto antes de tudo)

Estas cinco decisões definem 90% do projeto. Se você acertar nelas, o resto é execução.

### 1.1 O core emite um *trace de eventos*, não um "estado atual"

O erro clássico é fazer o C devolver o estado da estrutura depois de cada operação e o frontend redesenhar tudo. Isso dá uma foto, não uma animação — e você perde exatamente o que torna o projeto didático.

**O core executa a operação e emite a lista de micro-passos que aconteceram.** O frontend recebe esse trace e reproduz na velocidade que quiser, com play/pause/step/voltar.

```
JS: ds_call(OP_PUSH, 42)
C : executa pilha_push(p, 42), emitindo eventos pelo caminho
    → [NODE_NEW id=7 val=42] [EDGE_SET 7→3] [PTR_SET topo=7] [COUNT ops+1]
JS: aplica os eventos ao seu modelo visual, um a cada N ms, com tween
```

Consequência boa: **o código em C continua sendo C idiomático de matéria**, com uma macro `TR(...)` espalhada. Você não escreve código de desenho em C nenhuma vez.

### 1.2 Zero parser de JSON dentro do C

Tentação: mandar comandos como JSON e devolver estado como JSON. Não faça — você vai escrever um parser de JSON em C e vai ser a parte mais chata e mais bugada do projeto.

**Protocolo real:**

| Direção | Formato |
|---|---|
| JS → C | `ds_call(int op, int a, int b, int c)` — `op` é um enum inteiro |
| JS → C (dados em massa) | JS escreve direto na heap do WASM, passa ponteiro + tamanho |
| C → JS | buffer binário de eventos (`Int32Array` visto direto na heap, sem cópia nem parse) |

Nenhuma serialização de texto na fronteira. O "estado atual" nunca é transmitido: o **JS mantém o próprio modelo** aplicando os eventos. O C é a fonte da verdade lógica, o JS é a fonte da verdade visual.

### 1.3 O mesmo core compila nativo e para WASM

`core/` não conhece Emscripten. Compila com MinGW para rodar os testes e o CLI localmente (loop de desenvolvimento em milissegundos, com AddressSanitizer), e compila com `emcc` para o navegador. Só o arquivo `core/api/api.c` tem `#ifdef __EMSCRIPTEN__`.

Isso te dá de graça: testes rápidos, debug com gdb, e um binário de terminal que serve para os trabalhos da matéria.

### 1.4 Cada evento carrega o arquivo e a linha do C que o gerou

```c
TR(EV_ARR_COMPARE, .a = j, .b = j + 1);   // grava __LINE__ e o id do arquivo
```

O frontend importa os `.c` como texto (`import src from './bolha.c?raw'`) e **destaca a linha real do código-fonte** enquanto anima. Não é pseudocódigo inventado — é o código que está rodando. É a feature que mais impressiona em portfólio e custa quase nada.

### 1.5 Voltar no tempo = reexecutar os eventos, não desfazer

Implementar o inverso de cada evento é trabalho dobrado e fonte de bugs sutis. Como aplicar evento é uma operação trivial (dezenas de milhares por frame), **para ir ao passo `k`, zere o modelo e aplique os eventos `0..k`**. Se algum dia ficar lento, guarde um keyframe (cópia do modelo) a cada 500 eventos.

---

## 2. Arquitetura

```
┌───────────────────────────────────────────────────────────────┐
│  web/  —  Vite + TypeScript + React (chrome) + Canvas 2D      │
│                                                               │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │  Player    │  │  Model       │  │  Renderers           │   │
│  │ play/pause │→ │ nós, arestas │→ │ ArrayView            │   │
│  │ step/scrub │  │ array, ptrs  │  │ NodeGraphView        │   │
│  │ velocidade │  │ contadores   │  │ (layout + tween)     │   │
│  └────────────┘  └──────────────┘  └──────────────────────┘   │
│         ▲                ▲                                    │
│         │  eventos       │  aplica evento por evento          │
└─────────┼────────────────┼────────────────────────────────────┘
          │ Int32Array view sobre a heap do WASM (zero-copy)
┌─────────┴─────────────────────────────────────────────────────┐
│  core/  —  C99, zero dependências                             │
│                                                               │
│   api/     ds_call(op,a,b,c) · ds_trace_ptr() · ds_trace_len()│
│   trace/   buffer de eventos · idmap (ponteiro→id estável)    │
│   ds/      pilha · fila · lista · abb · avl · hash · btree    │
│   sort/    bolha · insercao · selecao · quick · merge · ext   │
└───────────────────────────────────────────────────────────────┘
          │ mesmo core, sem Emscripten
┌─────────┴─────────────────────────────────────────────────────┐
│  cli/     binário de terminal (debug + trabalhos da matéria)  │
│  tests/   unitários + invariantes + fuzz diferencial          │
└───────────────────────────────────────────────────────────────┘
```

---

## 3. O sistema de trace (o coração do projeto)

### 3.1 Estrutura do evento

Tamanho fixo, sem ponteiros, sem strings — assim o JS lê o array inteiro como `Int32Array` sem parsear nada.

```c
/* core/trace/trace.h */
typedef struct {
    int32_t kind;   /* ev_kind                                    */
    int32_t src;    /* id do arquivo-fonte (enum SRC_*)           */
    int32_t line;   /* __LINE__                                   */
    int32_t a, b, c;/* operandos genéricos, significado por kind  */
} ev_t;             /* 24 bytes                                   */
```

### 3.2 Vocabulário de eventos

Um conjunto pequeno cobre **todas** as estruturas. Resista à tentação de criar um evento por estrutura.

```c
typedef enum {
    /* ---- genéricos ---------------------------------------- */
    EV_MSG,        /* a = id da mensagem (STR_*) — nunca texto  */
    EV_COUNT,      /* a = id do contador, b = delta             */
    EV_PHASE,      /* a = id da fase (ex.: "particionando")     */

    /* ---- mundo "array" (ordenação, busca, hash aberto) ----- */
    EV_ARR_INIT,   /* a = n                                     */
    EV_ARR_READ,   /* a = i                                     */
    EV_ARR_COMPARE,/* a = i, b = j                              */
    EV_ARR_SWAP,   /* a = i, b = j                              */
    EV_ARR_WRITE,  /* a = i, b = valor                          */
    EV_ARR_RANGE,  /* a = lo, b = hi  (destaca subarray)        */
    EV_ARR_MARK,   /* a = i, b = tag (PIVO, ORDENADO, MIN...)   */
    EV_AUX_INIT,   /* a = n  (buffer auxiliar do merge)         */
    EV_AUX_WRITE,  /* a = i, b = valor                          */

    /* ---- mundo "grafo de nós" (listas, árvores) ------------ */
    EV_NODE_NEW,   /* a = id, b = valor                         */
    EV_NODE_FREE,  /* a = id                                    */
    EV_NODE_SET,   /* a = id, b = slot da chave, c = valor      */
    EV_EDGE_SET,   /* a = id origem, b = slot, c = id destino   */
    EV_PTR_SET,    /* a = id do ponteiro nomeado, b = id do nó  */
    EV_VISIT,      /* a = id  (destaca nó sob o cursor)         */
    EV_UNVISIT,    /* a = id                                    */

    /* ---- memória secundária -------------------------------- */
    EV_DISK_READ,  /* a = id da página                          */
    EV_DISK_WRITE, /* a = id da página                          */

    EV_KIND_COUNT
} ev_kind;
```

> `EV_NODE_SET` com `slot` e `EV_EDGE_SET` com `slot` são o que permite reaproveitar o mesmo vocabulário para nó de lista (1 chave, 1–2 ponteiros), nó de ABB (1 chave, 2 filhos) e nó de árvore B (2t−1 chaves, 2t filhos). Vale muito a pena.

### 3.3 A macro

```c
/* no topo de cada .c instrumentado */
#define TR_SRC SRC_QUICKSORT

#define TR(k, ...) \
    trace_push((ev_t){ .kind = (k), .src = TR_SRC, .line = __LINE__, __VA_ARGS__ })
```

### 3.4 Como fica o código instrumentado

Este é o teste de sanidade do design: o algoritmo tem que continuar legível.

```c
#define TR_SRC SRC_BOLHA

void bolha(int *v, int n) {
    for (int i = 0; i < n - 1; i++) {
        int trocou = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            TR(EV_ARR_COMPARE, .a = j, .b = j + 1);
            if (v[j] > v[j + 1]) {
                int t = v[j]; v[j] = v[j + 1]; v[j + 1] = t;
                TR(EV_ARR_SWAP, .a = j, .b = j + 1);
                trocou = 1;
            }
        }
        TR(EV_ARR_MARK, .a = n - 1 - i, .b = TAG_ORDENADO);
        if (!trocou) { TR(EV_MSG, .a = STR_JA_ORDENADO); break; }
    }
}
```

```c
#define TR_SRC SRC_PILHA

int pilha_push(Pilha *p, int valor) {
    No *novo = malloc(sizeof *novo);
    if (!novo) return ERR_SEM_MEMORIA;
    novo->valor = valor;
    novo->prox  = p->topo;
    TR(EV_NODE_NEW,  .a = id_de(novo), .b = valor);
    TR(EV_EDGE_SET,  .a = id_de(novo), .b = 0, .c = id_de(p->topo));
    p->topo = novo;
    p->n++;
    TR(EV_PTR_SET,   .a = PTR_TOPO, .b = id_de(novo));
    TR(EV_COUNT,     .a = CNT_TAMANHO, .b = +1);
    return OK;
}
```

### 3.5 IDs estáveis para os nós (`idmap`)

O JS não pode ver ponteiros. Cada nó precisa de um inteiro estável, e endereços de `malloc` são reciclados depois do `free` — se você usar o endereço como id, um nó novo pode herdar a identidade visual de um nó morto e a animação vai fazer coisas absurdas.

Duas opções:

- **(recomendada) Registro ponteiro→id** em `core/trace/idmap.c`: tabela hash pequena, `id_de(void*)` cria um id novo se não existir, `id_esquece(void*)` remove no `free`. Vantagem: as `struct` das estruturas ficam **idênticas às da matéria**, sem campo extra de visualização.
- **(mais simples) Campo `int id;`** dentro de cada nó, preenchido de um contador global. Menos código, mas suja a struct didática.

`id_de(NULL)` deve devolver `0`, e `0` significa NULL no frontend. Isso simplifica `EV_EDGE_SET`.

### 3.6 Buffer e limites

```c
#define TRACE_CAP  (1 << 18)   /* 262144 eventos ≈ 6 MB */

static ev_t   g_ev[TRACE_CAP];
static int32_t g_n;
static int32_t g_truncado;

void        trace_reset(void);
void        trace_push(ev_t e);   /* no-op silencioso se cheio, marca g_truncado */
int32_t     trace_len(void);
const ev_t* trace_ptr(void);
int32_t     trace_truncado(void);
void        trace_set_enabled(int on);  /* desligado nos benchmarks */
```

`trace_set_enabled(0)` é essencial para a feature de "medir comparações para n grande" — você roda o quicksort com n = 100 000 sem gerar 2 milhões de eventos.

---

## 4. Fronteira C ↔ WASM

### 4.1 API exportada (arquivo único: `core/api/api.c`)

```c
#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
  #define API EMSCRIPTEN_KEEPALIVE
#else
  #define API
#endif

/* --- sessão ------------------------------------------------ */
API void    ds_sessao_nova(int32_t tipo);   /* TIPO_PILHA, TIPO_AVL, ... */
API void    ds_sessao_fim(void);

/* --- execução ---------------------------------------------- */
API int32_t ds_call(int32_t op, int32_t a, int32_t b, int32_t c);
API int32_t ds_erro(void);                  /* código do último erro   */

/* --- dados em massa (JS escreve na heap e passa o ponteiro) - */
API int32_t* ds_buffer(void);               /* buffer de entrada       */
API int32_t  ds_buffer_cap(void);
API void     ds_carregar(int32_t n);        /* lê n ints do buffer     */

/* --- leitura do trace (zero-copy) -------------------------- */
API const ev_t* ds_trace_ptr(void);
API int32_t     ds_trace_len(void);
API int32_t     ds_trace_truncado(void);

/* --- benchmark sem trace ----------------------------------- */
API int32_t ds_bench(int32_t alg, int32_t n, int32_t seed, int32_t metrica);
```

### 4.2 Ciclo de uma operação, no JS

```ts
export function exec(op: Op, a = 0, b = 0, c = 0): Ev[] {
  const rc = mod._ds_call(op, a, b, c);
  if (rc < 0) throw new DsError(mod._ds_erro());

  const ptr = mod._ds_trace_ptr();
  const len = mod._ds_trace_len();

  // ⚠️ recriar a view SEMPRE: ALLOW_MEMORY_GROWTH desanexa buffers antigos
  const raw = new Int32Array(mod.HEAP32.buffer, ptr, len * 6);

  const evs: Ev[] = new Array(len);
  for (let i = 0; i < len; i++) {
    const o = i * 6;
    evs[i] = { kind: raw[o], src: raw[o+1], line: raw[o+2],
               a: raw[o+3], b: raw[o+4], c: raw[o+5] };
  }
  return evs;
}
```

> **Armadilha nº 1 do Emscripten:** com `-sALLOW_MEMORY_GROWTH=1`, qualquer `malloc` que cresça a heap invalida todas as `TypedArray` criadas antes. Guardar a view em uma variável de módulo é um bug garantido, e ele aparece só quando o dataset cresce. Recrie a view depois de cada chamada.

### 4.3 Flags do `emcc`

```
-O3 -flto
-sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web
-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16MB
-sEXPORTED_RUNTIME_METHODS=['HEAP32','HEAPU8','UTF8ToString']
-sEXPORTED_FUNCTIONS=['_ds_sessao_nova','_ds_call',...,'_malloc','_free']
-sSTACK_SIZE=1MB          # quicksort/mergesort recursivos precisam
--closure 1               # só no release
```

Build de debug separado com `-O0 -g3 -sASSERTIONS=2 -sSAFE_HEAP=1`.

---

## 5. Estrutura de pastas

```
edviz/
├── README.md                 ← com GIFs; é a capa do portfólio
├── CMakeLists.txt
├── .github/workflows/ci.yml
│
├── core/
│   ├── include/ds/           ← headers públicos
│   │   ├── pilha.h  fila.h  lista.h  abb.h  avl.h  hash.h  btree.h
│   │   ├── sort.h   trace.h  ids.h   erros.h
│   ├── trace/
│   │   ├── trace.c           ← buffer de eventos
│   │   ├── idmap.c           ← ponteiro → id estável
│   │   └── ids.h              ← enums compartilhados (§5.1)
│   ├── ds/
│   │   ├── linear.h              ← o vtable TAD_Linear (§8.3)
│   │   ├── pilha_enc.c  pilha_vet.c
│   │   ├── fila_enc.c   fila_vet.c   ← a com vetor é a circular
│   │   ├── lista_simples.c  lista_dupla.c  lista_circular.c  deque.c
│   │   ├── abb.c  avl.c  hash_encadeado.c  hash_aberto.c
│   │   └── btree.c  bplus.c  paginador.c
│   ├── sort/
│   │   ├── bolha.c  selecao.c  insercao.c  shell.c
│   │   ├── quick.c  merge.c  heap.c
│   │   └── intercalacao_externa.c
│   └── api/
│       └── api.c             ← único arquivo que sabe da existência do WASM
│
├── cli/
│   └── main.c                ← menu de terminal usando o mesmo core
│
├── tests/
│   ├── runner.c              ← runner mínimo próprio (~60 linhas)
│   ├── test_pilha.c  test_lista.c  test_abb.c  test_avl.c
│   ├── test_hash.c   test_btree.c  test_sort.c
│   ├── test_invariantes.c    ← propriedades: ordenado, permutação, balanceamento
│   └── test_fuzz.c           ← diferencial contra modelo de referência
│
└── web/
    ├── index.html
    ├── vite.config.ts
    ├── src/
    │   ├── wasm/             ← saída do emcc (gerada, no .gitignore)
    │   ├── core/
    │   │   ├── bridge.ts     ← carrega o módulo, expõe exec()
    │   │   ├── ops.ts        ← enums espelhados do C (gerados! ver §5.1)
    │   │   └── player.ts     ← play/pause/step/scrub/velocidade
    │   ├── model/
    │   │   ├── arrayModel.ts
    │   │   ├── graphModel.ts
    │   │   └── apply.ts      ← evento → mutação do modelo
    │   ├── render/
    │   │   ├── canvas.ts     ← camada base, DPR, resize
    │   │   ├── arrayView.ts
    │   │   ├── graphView.ts
    │   │   ├── layoutArvore.ts   ← Reingold–Tilford
    │   │   └── tween.ts
    │   ├── ui/               ← React: abas, controles, painéis
    │   └── content/          ← textos, complexidades, explicações
    └── public/
```

### 5.1 Um detalhe que evita muita dor: gerar os enums

Os enums `op`, `ev_kind`, `TAG_*`, `SRC_*` existem nos dois lados. Manter na mão **vai** dessincronizar e produzir bugs mudos (a animação faz a coisa errada, nada quebra).

Solução barata: um script `tools/gen_enums.py` que lê `core/include/ds/ids.h` e cospe `web/src/core/ops.ts`. Roda no build. Cinquenta linhas de Python que te salvam de uma tarde de depuração.

Ele gera também a lista de `STR_*` — os ids de mensagem que o `EV_MSG` carrega. Isso vira a **chave dos arquivos de tradução** (§7.6): se você adicionar um `STR_` novo no C e esquecer de traduzir, o build reclama em vez de a interface mostrar `undefined`.

---

## 6. Build e ambiente (Windows / MinGW / PowerShell)

### 6.1 Ferramentas

| Ferramenta | Para quê | Como |
|---|---|---|
| MinGW-w64 (já tem) | build nativo, testes, CLI | ok |
| CMake + Ninja | orquestração dos dois builds | `winget install Kitware.CMake Ninja-build.Ninja` |
| Emscripten SDK | build WASM | `git clone emsdk && .\emsdk install latest && .\emsdk activate latest` |
| Node 20+ / pnpm | frontend | `winget install OpenJS.NodeJS.LTS` |

O emsdk funciona bem no Windows nativo — não precisa de WSL. No PowerShell, `emsdk_env.ps1` ajusta o PATH da sessão.

### 6.2 Os dois builds

```powershell
# nativo: testes + CLI, rápido, com sanitizers
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDS_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure

# wasm: saída direto para dentro do web/src/wasm/
emcmake cmake -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm

# frontend
cd web; pnpm install; pnpm dev
```

Um script `tools/dev.ps1` que roda o build wasm em watch e o `vite dev` em paralelo economiza muito atrito no dia a dia.

### 6.3 CI (GitHub Actions)

Três jobs, e eles são parte do portfólio tanto quanto o código:

1. **nativo** — build + `ctest` + ASAN/UBSAN em Linux e Windows.
2. **wasm** — build com emsdk, falha se algum símbolo exportado sumir.
3. **deploy** — build do Vite + publish no GitHub Pages (só na `main`).

O projeto ser 100% estático (WASM, sem servidor) é o que torna o deploy trivial e o link do Pages compartilhável.

---

## 7. Frontend

### 7.1 Stack

- **Vite + TypeScript** — build rápido, importar `.c?raw` e `.wasm` sai de graça.
- **React** apenas para o *chrome* da aplicação (abas, botões, formulários, painéis). É o que mais aparece em vaga.
- **Canvas 2D puro** para a visualização, isolado em classes que o React não gerencia. React não deve saber que existe um canvas além de uma `<canvas ref>`.
- **Sem biblioteca de gráficos** para as estruturas. Para o gráfico de complexidade empírica (§9.4), uma lib leve tipo `uPlot` é aceitável.

Regra de ouro: **estado da animação fora do React**. Um `Player` com `requestAnimationFrame` mutando um modelo e desenhando. O React só recebe, por `useSyncExternalStore` ou um estado throttled, o que precisa aparecer nos painéis (passo atual, contadores, linha destacada) — a ~15 Hz, não a 60.

### 7.2 O Player

```ts
class Player {
  private eventos: Ev[] = [];
  private i = 0;                    // índice do próximo evento
  private vel = 1;                  // multiplicador
  private tocando = false;

  carregar(evs: Ev[]): void          // reseta modelo e vai para o passo 0
  irPara(k: number): void            // reseta e aplica 0..k  (§1.5)
  passo(delta: +1 | -1): void
  play(): void; pause(): void
  setVelocidade(v: number): void     // 0.25× … 16×
  get progresso(): { i: number; total: number }
}
```

Atalhos de teclado desde o começo: `espaço` play/pause, `←`/`→` passo, `shift+←/→` pular 10, `r` reiniciar. Custa nada e muda a sensação do produto.

### 7.3 Renderizadores

**`ArrayView`** — barras ou células. Estados visuais por índice: normal, lendo, comparando, trocando, pivô, dentro da partição ativa, ordenado. Um array paralelo de "estado visual" por índice, reconstruído junto com o modelo.

**`GraphView`** — nós e arestas. Duas responsabilidades separadas:

- **Layout** — calcula a posição *alvo* de cada nó.
  - Lista: cadeia horizontal com quebra de linha.
  - ABB / AVL: **Reingold–Tilford** (não use `x = pai.x ± largura/2^nivel`; com árvore degenerada os nós se sobrepõem e fica feio).
  - Árvore B / B+: mesmo algoritmo, mas cada nó é um retângulo com várias chaves, e a largura do nó entra no cálculo.
- **Tween** — cada nó tem posição atual e alvo; a cada frame, `atual += (alvo - atual) * 0.2`. Só isso já faz **rotação de AVL e split de árvore B ficarem lindos**, sem nenhuma lógica de animação específica. Esse é o truque que mais rende visualmente por linha de código.

Detalhes que separam "trabalho de faculdade" de "portfólio": `devicePixelRatio` (senão fica borrado em tela 4K), redesenhar só quando algo mudou, `prefers-reduced-motion`, paleta que funcione em dark mode e para daltônicos (não codifique informação só por cor — use também borda/hachura).

### 7.4 Tokens de design

Fechados no canvas de design. Copie isto para `web/src/styles/tokens.css` e leia os mesmos valores no renderizador (`getComputedStyle(root).getPropertyValue('--st-compare')`), para não ter duas paletas divergindo.

```css
:root {
  /* superfícies */
  --bg-0:      #0b0c10;  /* fundo da página                 */
  --bg-1:      #111317;  /* painéis e cartões               */
  --bg-2:      #191c21;  /* controles em repouso            */
  --bg-3:      #21252b;  /* hover e seleção                 */
  --canvas:    #07080b;  /* superfície de visualização      */
  /* traço e tinta */
  --line:      #272a30;
  --line-2:    #3e4148;
  --fg:        #f2f3f6;
  --fg-2:      #a6a9b0;
  --fg-3:      #71747c;
  /* acento — interface apenas */
  --accent:    #6a99ff;
  --accent-hi: #95bcff;
  --accent-dim:#314c8c;
  /* estados do algoritmo */
  --st-compare: #b98a00;
  --st-swap:    #ae4440;
  --st-done:    #39ac6d;
  --st-pivot:   #885cb5;
  --st-aux:     #0085ca;
  /* identidade de algoritmo — só linhas e rótulos */
  --alg-1: #5889e6;  --alg-2: #c97500;  --alg-3: #00a890;
}
```

**Tipografia:** Space Grotesk (display) · Instrument Sans (interface) · JetBrains Mono (código, chaves, todo contador).

**Mapeamento estado → evento.** É isto que impede o renderizador de inventar cor:

| Estado | Token | Evento que dispara | Traço |
|---|---|---|---|
| Em repouso | `--bg-2` | — | sólido, `--line-2` |
| Comparando | `--st-compare` | `EV_ARR_COMPARE` | sólido |
| Trocando / removendo | `--st-swap` | `EV_ARR_SWAP`, `EV_NODE_FREE` | sólido |
| Ordenado / nó novo | `--st-done` | `EV_ARR_MARK`, `EV_NODE_NEW` | sólido |
| Pivô / chave que sobe | `--st-pivot` | `EV_ARR_MARK` | sólido |
| Auxiliar / página de disco | `--st-aux` | `EV_AUX_WRITE`, `EV_DISK_READ` | **tracejado** |
| Cursor / visitado | `--accent` | `EV_VISIT` | sólido, 2px |
| Fora da partição | `--bg-2` a 45% | `EV_ARR_RANGE` | tracejado |

Três regras que valem repetir, porque são o que segura a coerência:

1. **O acento é da interface** — foco, seleção, transporte, botão primário. Ele nunca preenche uma célula nem um nó. A única exceção é o cursor, e é justamente por ser "onde a interface está olhando".
2. **Identidade de algoritmo é linha fina; estado é preenchimento.** Na aba de ordenação, o quicksort é uma régua de 3px no cabeçalho e uma linha de 2px no gráfico — nunca a cor de uma barra. A barra pertence ao estado.
3. **Nenhum estado é distinguido só por matiz.** Cada um tem também um tratamento de traço, e a legenda fica sempre visível sob o canvas. Isso não é preciosismo: a paleta foi validada para protanopia e deuteranopia contra o fundo `--canvas`, e é o traço que segura os pares mais próximos.

> Detalhe que só aparece na hora de implementar: pivô e buffer auxiliar **nunca** aparecem juntos na tela (um é do quicksort, o outro do mergesort). É o que permite manter seis estados sem estourar os limites de separação para daltonismo. Se um dia você puser os dois na mesma cena, revalide a paleta.

### 7.5 Painel de código-fonte

```ts
import fonteBolha from '../../core/sort/bolha.c?raw';
```

Mapa `SRC_* → texto`, destaque na linha `ev.line`, auto-scroll suave. Vale exibir também um contador de execuções por linha (um "profiler" visual) — barato de fazer somando por linha e visualmente muito rico.

### 7.6 Interface em dois idiomas

Português e inglês. Isso parece uma tarefa de acabamento, e é justamente por pensarem assim que os projetos se enrolam: **i18n é decisão de Fase 0, não feature de Fase 6.** Retrofitar significa caçar string literal em cinquenta componentes. A regra desde o primeiro commit custa nada:

> Nenhum texto voltado ao usuário aparece literal num componente. Tudo passa por `t('chave')`.

**Consequência boa: o C fica mais simples do que estava planejado.** A versão anterior deste plano tinha um `ds_strings()` devolvendo uma arena de strings do núcleo. Com dois idiomas isso vira um problema — ou você mantém duas arenas em C, ou tem português cravado dentro do core. A saída é melhor que o problema: **o C nunca devolve texto.**

```c
TR(EV_MSG, .a = STR_ROT_DIREITA);   /* um id, não uma frase */
```

```ts
// web/src/i18n/pt.ts
export default {
  STR_ROT_DIREITA: 'rotação simples à direita',
  STR_FILA_CHEIA:  'fila cheia — n == cap',
  'op.inserir':    'Inserir',
  'metrica.comparacoes': 'comparações',
};
```

O `ds_strings()` da §4.1 sai da API. Menos código no C, e o núcleo fica agnóstico de idioma — que é o que ele já deveria ser.

**O que o i18n cobre e o que não cobre:**

| Cobre | Não cobre |
|---|---|
| rótulos, botões, nomes de estrutura, log, textos explicativos | o **código-fonte** no painel lateral |
| mensagens do `EV_MSG` (via `STR_*`) | nomes de arquivo (`avl.c`) |
| formatação de número (`163,8 M` ↔ `163.8 M`, `25 600` ↔ `25,600`) via `Intl.NumberFormat` | identificadores e comentários no C |

**Isso levanta uma escolha que vale fazer agora, porque muda todo o core:** em que idioma se escreve o C? Interface em inglês com painel mostrando `rebalancear(No *n)` e `/* esquerda-esquerda */` fica pela metade.

**Decidido: o C é escrito em português** — `rebalancear`, `rot_direita`, `no->esq`, `/* esquerda-esquerda */`. O painel de código mostra o mesmo texto nos dois idiomas, e é assim mesmo: código é código, e um leitor de qualquer língua acompanha a estrutura. Em troca, o repositório fala a língua da matéria — o que também é o argumento mais honesto para um projeto de graduação.

Duas compensações que custam pouco e resolvem o desconforto do leitor internacional:

- **README em inglês** (ou pt/en lado a lado). É a única página que recrutador de fora lê de verdade.
- **Um glossário de dez linhas** no README mapeando os termos: `no` → node, `esq/dir` → left/right, `fb` → balance factor, `percurso` → traversal, `enfileirar` → enqueue. Resolve 90% da fricção por um custo de cinco minutos.

**Detalhes de implementação, todos baratos:**

- Idioma inicial de `navigator.language`, com override salvo em `localStorage`.
- O idioma entra na URL (`?lang=en`), junto do estado compartilhável da §8.4 — um link enviado abre no idioma de quem enviou.
- Um teste que compara as chaves de `pt.ts` e `en.ts` e falha se divergirem. Cinco linhas, e é o que impede a versão em inglês de apodrecer.
- Cuidado com texto dentro do `<canvas>`: rótulos como `início`/`fim` desenhados no vetor também passam pelo `t()`. É o lugar mais fácil de esquecer.

---

## 8. Aba 1 — Simulador de Estruturas

### 8.1 Layout da tela

```
┌──────────────────────────────────────────────────────────────┐
│  [Estruturas]  [Ordenação]                          ☾ ⓘ GH  │
├────────────┬─────────────────────────────────┬───────────────┤
│ Estrutura  │                                 │  Código-fonte │
│ ○ Pilha    │                                 │   3 │ No *n = │
│ ● Lista ↔  │        C A N V A S              │ ▸ 4 │   n->pr │
│ ○ ABB      │      (nós + ponteiros)          │   5 │   p->to │
│ ○ AVL      │                                 │               │
│ ○ Hash     │                                 ├───────────────┤
│ ○ Árvore B │                                 │  Métricas     │
│            │                                 │  tamanho    7 │
├────────────┤                                 │  altura     3 │
│ Operações  │                                 │  compar.   12 │
│ [inserir▸] ├─────────────────────────────────┤  aloc.      7 │
│ [remover▸] │ ⏮ ◀ ▶ ⏭  ━━━━●━━━━  1×  32/118 │               │
│ [buscar ▸] ├─────────────────────────────────┴───────────────┤
│ [limpar  ] │ ▸ compara 42 com 17 → vai para a direita        │
│ [aleatório]│ ▸ nó 7 criado                                   │
└────────────┴─────────────────────────────────────────────────┘
```

### 8.2 Estruturas e operações

| Grupo | Estrutura | Operações | Métricas exibidas |
|---|---|---|---|
| Lineares | **Pilha** — encadeada e com vetor | push, pop, topo, limpar | tamanho, alocações, escritas, capacidade |
| | **Fila** — encadeada e com vetor (circular) | enfileirar, desenfileirar, frente | idem + início/fim, ocupação |
| | Lista simples | inserir início/fim/posição, remover, buscar | tamanho, comparações |
| | Lista dupla | idem, com os dois ponteiros desenhados | tamanho |
| | Lista circular | idem | tamanho |
| Busca em RAM | Busca sequencial | buscar | comparações |
| | Busca binária | buscar (exige vetor ordenado) | comparações, intervalo |
| | ABB | inserir, remover (3 casos!), buscar, percursos | altura, nós, comparações |
| | AVL | idem + **rotações destacadas** | altura, FB por nó, nº de rotações |
| | Hash encadeado | inserir, buscar, remover, mudar função hash | fator de carga, colisões, maior cadeia |
| | Hash aberto | idem + sondagem linear/quadrática/dupla | agrupamento, sondagens |
| Memória sec. | Árvore B | inserir (split!), remover (merge!), buscar | ordem t, altura, **acessos a disco** |
| | Árvore B+ | idem + lista ligada das folhas | idem + varredura sequencial |

Detalhes que valem esforço extra porque são exatamente onde os alunos travam:
- **remoção em ABB com dois filhos** — mostrar a busca do sucessor em ordem, passo a passo;
- **rotações de AVL** — pausar antes e depois, com rótulo `rotação dupla esquerda-direita`;
- **split de nó em árvore B** — a chave que sobe é o momento crítico;
- **contador de acessos a disco** na aba de memória secundária — é o único jeito de a diferença entre B e B+ ficar concreta.

### 8.3 Duas implementações atrás do mesmo TAD

Pilha e fila entram com as **duas** implementações: alocação dinâmica e vetor. Isso não é escopo dobrado — é o argumento central da primeira metade da ementa virando imagem. E dá de graça a coisa que nenhum visualizador da internet faz: mostrar as duas rodando a **mesma sequência de operações** lado a lado.

**Um header, duas implementações.** É literalmente a definição de TAD que a matéria dá — interface separada da implementação:

```c
/* core/include/ds/fila.h — o TAD, uma interface só */
typedef struct Fila Fila;

Fila *fila_criar(int capacidade);   /* capacidade ignorada na versão encadeada */
int   fila_enfileirar(Fila *f, int valor);
int   fila_desenfileirar(Fila *f, int *saida);
int   fila_frente(const Fila *f, int *saida);
int   fila_tamanho(const Fila *f);
void  fila_destruir(Fila *f);
```

Como as duas compilam no mesmo binário, elas não podem definir os mesmos símbolos. Prefixe (`fila_vet_*`, `fila_enc_*`) e despache por uma tabela — que é, ela própria, uma boa demonstração:

```c
/* core/ds/linear.h */
typedef struct {
    const char *nome;
    void *(*criar)(int cap);
    int   (*inserir)(void *s, int v);
    int   (*remover)(void *s, int *saida);
    int   (*tamanho)(const void *s);
    void  (*destruir)(void *s);
} TAD_Linear;

extern const TAD_Linear FILA_VET, FILA_ENC, PILHA_VET, PILHA_ENC;
```

`ds_sessao_nova(TIPO_FILA_VET)` só escolhe qual `TAD_Linear` a sessão aponta. Trocar de implementação em tempo de execução vira uma linha.

**Nenhum evento novo é preciso.** Este é o teste que o vocabulário da §3.2 tinha que passar, e passa:

| | encadeada | com vetor |
|---|---|---|
| criar elemento | `EV_NODE_NEW` | `EV_ARR_WRITE` |
| ligar | `EV_EDGE_SET` | — (a posição é implícita) |
| ponteiro nomeado | `EV_PTR_SET .b = id do nó` | `EV_PTR_SET .b = índice` |
| destruir | `EV_NODE_FREE` | `EV_ARR_MARK` (posição liberada) |

`EV_PTR_SET` serve aos dois mundos sem mudar de forma: num, `.b` é o id de um nó; no outro, é um índice. O renderizador sabe qual desenhar pelo tipo da sessão. Se você tivesse criado eventos específicos por estrutura lá atrás, aqui teria que criar mais seis.

**O que cada implementação torna visível** — e é por isso que ver as duas vale mais que ver uma:

| | encadeada | com vetor |
|---|---|---|
| o que dá para observar | `malloc`/`free` por elemento, o `NULL` no fim, ponteiros sendo religados | capacidade fixa, **overflow**, wrap-around, posições sobrando |
| o erro que ela ensina | vazamento ao desenfileirar sem `free` | confundir fila cheia com fila vazia |
| custo de memória | 16 B por elemento, esparso | 4 B por slot, contíguo, reservado de uma vez |

Dois momentos específicos que valem ser desenhados com cuidado, porque são exatamente onde a aula trava:

1. **O wrap-around da fila circular.** Com `início = 5` e `fim = 2` num vetor de 8, a fila *parece* invertida na tela. Mostre a ordem lógica (1º, 2º, 3º…) por cima das células, além dos índices físicos por baixo — é a única forma de as duas leituras coexistirem.
2. **`cheia` vs. `vazia`.** Sem o contador `n`, `fim == início` significa as duas coisas ao mesmo tempo. Deixe o `n` sempre visível e, no modo passo a passo, marque o instante em que `fim` alcança `início`.

**Modo comparar.** Um terceiro estado do seletor, ao lado de `encadeada` e `vetor`: a tela divide em duas faixas, a mesma sequência de operações alimenta as duas, e o mesmo `Player` controla ambas. Custa pouco (é `ds_call` em duas sessões, os dois traces reproduzidos no mesmo relógio) e é o argumento visual mais forte do projeto inteiro — vira GIF de README junto com o modo corrida da ordenação.

> Se essa dupla implementação te agradar, a extensão natural é **lista sequencial** (lista com vetor, com o deslocamento na inserção no meio) contra a lista encadeada — o mesmo par, agora mostrando o custo `O(n)` da inserção que a encadeada não tem. Não está no escopo acima; diga se quiser.

### 8.4 Modos de entrada

- Operação avulsa (campo + botão).
- **Script de operações**: caixa de texto com `inserir 5 / inserir 3 / remover 5`, executa tudo com pausa entre elas. Ótimo para reproduzir um exercício da lista da matéria.
- Geração aleatória com **seed fixa** (gerador próprio no C, não `rand()` — assim o mesmo seed dá a mesma cena em qualquer máquina, e você pode compartilhar por URL).
- Estado inicial na URL (`?e=avl&ops=i5,i3,i8,r5`) → link reproduzível. Feature barata, impressiona muito.

---

## 9. Aba 2 — Algoritmos de Ordenação

### 9.1 Escopo

| Algoritmo | O que destacar visualmente |
|---|---|
| Bolha (com flag de parada) | trocas adjacentes, cauda já ordenada |
| Seleção | mínimo corrente, fronteira ordenado/desordenado |
| Inserção | elemento em mãos, deslocamento à direita |
| Shellsort | os gaps (colorir por sub-sequência) |
| Quicksort | pivô, partição, recursão (pilha de intervalos ao lado) |
| Mergesort | árvore de divisão em cima, buffer auxiliar embaixo |
| Heapsort *(opcional)* | o vetor **e** a árvore, lado a lado |
| Intercalação externa | blocos/fitas, memória limitada a k registros, passadas |

**Intercalação externa merece atenção** — é o tópico da ementa que praticamente nenhum visualizador na internet cobre. Visualize: vetor grande "no disco", uma janela de memória de k registros, geração dos runs iniciais, e cada passada de intercalação com o contador de acessos a disco. É o diferencial do projeto.

### 9.2 Controles

- Tamanho do vetor (5 … 200 no modo animado).
- Distribuição inicial: aleatório · **quase ordenado** · **inversamente ordenado** · **poucos valores distintos** (mata quicksort ingênuo) · manual.
- Seed.
- Velocidade e passo a passo (mesmo `Player` da outra aba — reuso total).

### 9.3 Modo corrida

Rodar N algoritmos sobre **o mesmo vetor inicial**, em painéis lado a lado, sincronizados por número de passos ou por tempo. É a demo mais compartilhável do projeto — o que vai virar o GIF do README.

### 9.4 Modo empírico (o que mostra maturidade)

Com `trace_set_enabled(0)` e `ds_bench()`, rode cada algoritmo para n = 100, 200, 400, … 25 600 e plote **comparações medidas × n**, com as curvas teóricas `n²` e `n log n` sobrepostas. Você sai de "eu implementei os algoritmos" para "eu medi e a curva bate com a teoria" — inclusive mostrando que a inserção **ganha** do quicksort em vetores pequenos ou quase ordenados. Isso é conteúdo de relatório de disciplina e de portfólio ao mesmo tempo.

---

## 10. Testes e qualidade

O core em C ser testável nativamente é o que permite tudo isto — e é o que um recrutador olha.

**Unitários** — runner próprio de ~60 linhas (`ASSERT_EQ`, `ASSERT_TRUE`, contagem de falhas). Não precisa de framework.

**Invariantes (o que mais pega bug):**

| Estrutura | Invariante verificada após cada operação |
|---|---|
| Ordenação | saída ordenada **e** é permutação da entrada (histograma) |
| ABB | percurso em ordem é crescente |
| AVL | ABB + `|FB| ≤ 1` em todo nó + altura ≤ 1,45·log₂(n+2) |
| Hash | tudo que foi inserido é encontrado; nada removido é encontrado |
| Árvore B (ordem t) | todas as folhas na mesma profundidade; `t−1 ≤ chaves ≤ 2t−1` (exceto raiz) |
| Listas | tamanho bate com a contagem por travessia; `dupla`: `n->prox->ant == n` |

**Fuzz diferencial** — gere 10 000 operações aleatórias e compare contra um modelo de referência trivial (um vetor ordenado, ou uma `std`-like ingênua). Qualquer divergência é bug. Poucas linhas, encontra os bugs de remoção que você jamais acharia à mão.

**Memória** — `-fsanitize=address,undefined` no CI. Uma função `ds_vazamentos()` que checa se todo `malloc` teve `free` depois de `limpar()` é fácil (o `idmap` já sabe quantos nós vivos existem) e é uma verificação real de qualidade.

**Frontend** — dá para pular testes unitários de React aqui sem prejuízo. O que vale é 3–4 testes do `apply.ts` (evento → modelo), que é onde bug silencioso mora.

---

## 11. Roadmap por fases

Cada fase termina com algo funcionando e comitado. Nada de "vou fazer todas as estruturas e depois plugo na tela".

### Fase 0 — Fundação · ~1 semana

- [x] Repositório, licença MIT, `.gitignore`, `.editorconfig`
- [x] CMake com os alvos `core`, `cli`, `tests`
- [x] `trace.c` + `idmap.c` + `ids.h` + script `gen_enums.py`
- [x] emsdk instalado; build wasm gerando `.js` + `.wasm`
- [x] Vite + TS + React inicializado, carregando o módulo WASM
- [x] **Camada `t()` de i18n com `pt.ts` e `en.ts`** — vazios, mas no lugar (§7.6)
- [x] CI: build nativo + testes + verificação de chaves de tradução
- **Pronto quando:** o botão na página chama uma função em C que devolve `42`.

### Fase 1 — Fatia vertical: a Pilha · ~1–2 semanas ⭐

**A fase mais importante do projeto.** Ela valida a arquitetura inteira. Se algo estiver errado no design, você descobre agora, com uma estrutura de 60 linhas, e não com sete estruturas prontas.

Só a versão **encadeada** aqui. A com vetor entra na Fase 2, e a razão é deliberada: a encadeada exercita `idmap`, `EV_NODE_NEW` e `EV_EDGE_SET`, que são a parte do trace com mais chance de estar mal desenhada. A com vetor é mais fácil e não testaria nada novo.

- [x] `pilha_enc.c` instrumentada (push, pop, topo, limpar)
- [x] `ds_call` + leitura do trace no JS
- [x] `graphModel` + `apply.ts` para os eventos de nó
- [x] `GraphView` desenhando nós, setas e o rótulo `topo`
- [x] Tween de posição
- [x] `Player` completo: play/pause/step/scrub/velocidade
- [x] Painel de código-fonte com destaque de linha
- [x] Painel de métricas e log
- [x] Testes de `pilha_enc` + invariantes
- **Pronto quando:** você dá push em cinco valores, arrasta a barra de progresso para trás e para a frente e a animação está correta nos dois sentidos.

### Fase 2 — Lineares completas, nas duas implementações · ~2 semanas

A ordem importa: o `ArrayView` nasce aqui, na pilha com vetor, e não na Fase 3. Quando a aba de ordenação chegar, ele já existe e já foi depurado numa estrutura de 40 linhas.

- [x] `linear.h` + o vtable `TAD_Linear`; `pilha_enc` migrada para ele
- [x] `pilha_vet.c` — com **overflow** visível quando `n == cap`
- [x] `arrayModel` + `ArrayView` (células, índices, ponteiros nomeados)
- [x] `fila_enc.c` e `fila_vet.c` — a com vetor é a circular
- [x] Wrap-around desenhado, com a ordem lógica por cima das células
- [x] Seletor de implementação na barra lateral: `encadeada | vetor | comparar`
- [x] **Modo comparar**: duas sessões, dois traces, um `Player` só
- [x] Lista simples, dupla, circular
- [x] Modo script de operações
- [x] Testes das quatro + fuzz diferencial (as duas implementações do mesmo TAD contra o mesmo modelo de referência — se divergirem, uma está errada)
- **Pronto quando:** a mesma sequência de operações roda nas duas implementações lado a lado, e os contadores de `malloc` e de escritas contam histórias diferentes.

> O fuzz diferencial fica especialmente barato aqui: as duas implementações têm a mesma interface, então você roda a mesma sequência aleatória nas duas e compara as saídas. Não precisa nem de modelo de referência — uma é o oráculo da outra.

### Fase 3 — Aba de ordenação · ~2 semanas

- [x] Eventos de array de ordenação — nenhum evento novo foi preciso; o `.c = 1`
      do `EV_ARR_COMPARE` cobriu o "valor em mãos" da inserção e do shell
- [x] Bolha, seleção, inserção, shell, quick, merge
- [x] Controles de distribuição e seed (e a distribuição manual, pelo `ds_buffer`)
- [x] Modo corrida — é o modo comparar da Fase 2, com algoritmos no lugar de
      implementações; o `Player` já aceitava as seis trilhas
- [x] `ds_bench` + gráfico empírico, em log-log
- [x] Testes: ordenado **e** permutação, para toda distribuição
- [x] O GIF do modo corrida no README — junto com o da ABB contra a AVL e o
      da árvore B contra a B+, cada um com o link que abre a cena de verdade
- **Pronto quando:** o GIF do modo corrida está no README.

> O `ArrayView` não foi reusado como o plano previa, e a razão apareceu na
> tela: ele desenha células com o número dentro, que é o desenho certo para uma
> pilha de oito posições — ali importa QUAL valor está em QUAL índice. Ordenar
> precisa de forma, e vinte células com número não viram forma. O `OrdenacaoView`
> desenha barras. O reúso que valia aconteceu uma camada abaixo, que é onde ele
> costuma valer: o `VetorModelo` e o `aplicar.ts` são os mesmos.

### Fase 4 — Busca em memória primária · ~2–3 semanas

- [x] Busca sequencial e binária — reusaram o `ArrayView` de fato, como o plano
      previa: elas entraram como **duas implementações do mesmo TAD**, e o modo
      comparar da Fase 2 as põe lado a lado sem uma linha de interface nova
- [x] ABB: inserir, buscar, remover (os três casos), percursos — a caixinha
      ficou para trás por uns meses; está feito desde o commit da ABB
- [x] Layout Reingold–Tilford, com um desvio deliberado: nó de filho único
      fica meio passo para o lado, senão a árvore degenerada desenharia uma
      linha vertical e esconderia a degeneração que a aba existe para mostrar
- [x] AVL com rotações destacadas e FB por nó — e no modo comparar, ao lado
      da ABB, o que era "mostra depois" virou "mostra ao mesmo tempo"
- [x] Hash encadeado e hash aberto (três sondagens) — as quatro na mesma
      família, e o modo comparar as põe lado a lado de uma vez
- [ ] Trocar a função hash por um seletor. Hoje ela é `k mod m` nas quatro,
      e o controle é o `m`: trocar 8 por 7 muda o desenho inteiro sem trocar
      uma linha de código, que é a metade da lição que importa. Um segundo
      seletor multiplicaria os TIPO_* por dois
- [x] Invariantes + fuzz de ABB/AVL
- **Pronto quando:** inserir uma sequência crescente na ABB e depois na AVL mostra visualmente por que a AVL existe.

### Fase 5 — Memória secundária · ~2 semanas

- [x] `paginador.c`: simula páginas de disco e conta acessos — sem cache, de
      propósito: um buffer pool deixaria a raiz sempre em memória e os números
      ficariam mais realistas e menos ensináveis
- [x] Árvore B: inserir com split, remover com merge/redistribuição
- [x] Árvore B+: folhas encadeadas, varredura sequencial — e a varredura só
      ensina se houver contra o que comparar, então a árvore B ganhou a dela
      junto: a mesma leitura em ordem, subindo e descendo, relendo o pai a
      cada volta. Com t = 3 e 500 chaves, 413 páginas contra 166
- [x] `CAMPO_FOLHA`: o nó anuncia se é folha, e numa folha o slot 0 deixa de
      ser filho e passa a ser o elo para a folha seguinte — que é o ponteiro
      que sobra numa página de verdade. Nenhum evento novo, como sempre
- [x] Intercalação externa na aba de ordenação — entrou como o sétimo
      algoritmo da mesma tabela, sem caso especial: a assinatura é a mesma, e
      o que muda é o que se conta. A memória `k` é o único parâmetro, e é ele
      que decide quantas varreduras do disco a ordenação custa. Com n = 64:
      k = 2 dá 6 passadas e 384 páginas; k = 8 dá 4 e 64; k = 64 dá 1 e 2
- [x] Fica fora do modo empírico, e de propósito: aquele gráfico plota
      COMPARAÇÕES contra n, e a métrica desta é passada — que nem varia com a
      distribuição que o gráfico varia. A curva cairia em cima da do mergesort
- [x] Contador de acessos a disco em destaque
- **Pronto quando:** buscar o mesmo conjunto em ABB e em árvore B mostra a diferença de acessos a disco em números.

### Fase 6 — Polimento e portfólio · ~1–2 semanas

- [x] `prefers-reduced-motion` respeitado — desde a Fase 1, nas três views
- [x] **Tema claro fica de fora, e é decisão, não pendência.** A paleta
      escura foi validada para protanopia e deuteranopia; um segundo tema é
      uma segunda validação inteira, e ela não é opcional — sem revalidar,
      um tema claro publicado é um tema que pode estar mentindo para quem
      mais depende da cor. Para um simulador que se usa em sala escura, o
      retorno não paga o risco
- [ ] **Responsivo: conhecido e não resolvido.** A grade pede ~980px, e
      abaixo disso as colunas da direita — código e log — ficam fora de
      alcance, porque `.app` corta o que transborda em vez de rolar. Num
      telefone dá para ver a lista e parte do canvas, e mais nada. A aba é
      uma ferramenta de mesa, mas o link do README é clicado no telefone:
      o mínimo honesto seria deixar rolar na horizontal em vez de cortar
- [x] Revisão final do inglês — as 334 chaves lidas em par com o português,
      procurando desvio de sentido e não só erro de digitação. Sete frases
      corrigidas; a pior era `STR_FUNDE`, que dizia "no sibling has room to
      spare" quando a fusão acontece justamente porque nenhum irmão tem CHAVE
      sobrando — o inglês dizia o contrário do C
- [x] E um defeito que a revisão do inglês pegou fora do dicionário: o `lang`
      do documento nascia "pt-BR" e só era corrigido na TROCA de idioma. Quem
      abria em inglês e não trocava — quase todo mundo — tinha a página lida
      por leitor de tela com voz portuguesa
- [x] Textos explicativos por estrutura, nos dois idiomas — três coisas por
      estrutura, e nenhuma delas é a definição: a tabela de complexidade com
      MÉDIO e PIOR em colunas separadas (é a distância entre os dois que a
      aula de função hash existe para explicar), «quando usar» em uma frase, e
      «a pega» — o preço, o erro clássico, ou a razão de a estrutura seguinte
      existir. A pega da ABB manda inserir 1 2 3 4 5 e olhar a altura, e o
      link do §8.4 faz exatamente isso
- [x] As complexidades não passam pelo i18n (`O(log n)` é igual nos dois
      idiomas) e os rótulos das operações vêm do próprio catálogo, então uma
      pilha diz «empilhar» e uma fila diz «enfileirar» sem a tabela saber
- [x] Estado compartilhável por URL, com o idioma junto — a barra de endereços
      é o link: cada aba grava o estado dela ali a cada mudança, e não existe
      botão "gerar link" (existe um "copiar", para quem não pensaria em olhar
      para cima). Os valores são SLUG e não número: `e=12` quebraria todo link
      compartilhado no dia em que alguém acrescentasse uma estrutura no meio
      do ids.h, e quebraria em silêncio
- [x] O link carrega as ENTRADAS, nunca o desenho — `ops=i1,i2,i3` e a semente
      —, e quem reconstrói a tela é o núcleo rodando de novo. É a regra da
      §1.5 outra vez, agora entre duas máquinas em vez de dois instantes
- [x] **Exportar GIF/PNG fica de fora, e é decisão.** O caso de uso é
      «quero mostrar isto para alguém», e o link compartilhável resolve
      melhor: ele abre a cena de verdade, com transporte e código do lado,
      em vez de um arquivo parado. Os três GIFs do README foram gravados
      por fora, uma vez, e é essa a frequência real da necessidade — um
      exportador de GIF em canvas é muito código para um botão que se usa
      uma vez na vida do projeto
- [x] README com GIFs — três, no topo, cada um com o link que abre a MESMA
      cena no demo. Gravados quadro a quadro pelo botão de passo, o que dá
      uma animação que avança um evento por quadro em vez de um vídeo
      acelerado; 12 MB de saída viraram 700 KB no gifsicle, com escala 0,62 e
      64 cores
- [x] Deploy no GitHub Pages — o mesmo `dist` que passou nos testes é o que
      vai ao ar, e o job só roda na main e só depois dos três de verificação.
      `base: "./"` no Vite, e não `/simux/`: com caminho relativo o mesmo
      bundle serve a raiz de um domínio, um subdiretório e um domínio próprio,
      que é a decisão que a §14.1 deixou em aberto. **Falta ligar
      Settings > Pages > Source = GitHub Actions**, que só o dono do
      repositório faz — sem isso o job falha com "Pages site not found"
- [x] Link do demo no topo do README, e a seção de decisões de arquitetura que
      a §13 põe em segundo lugar de retorno sobre esforço
- [x] Página "como funciona" explicando o trace e o WASM — virou a terceira
      aba, e a parte que vale não é o texto: a tabela de eventos é gerada NA
      HORA, chamando o núcleo de verdade. Uma pilha é criada, o 42 é
      empilhado, e o que a página mostra são os eventos que o segundo empilhar
      emitiu, com o arquivo e a linha do C ao lado. Embaixo, os mesmos eventos
      como o JavaScript os lê — seis inteiros cada um. Um exemplo escrito à
      mão envelheceria na primeira mudança de `pilha_enc.c`; este não
- [x] Os números do projeto na mesma página, tirados dos enums gerados do
      ids.h: 21 tipos de evento, 54 mensagens, 17 estruturas, 7 algoritmos

**Fase 6 fechada** com duas decisões de NÃO fazer — tema claro e exportador
de GIF — e uma pendência declarada: a largura mínima de ~980px. Fechar uma
fase com um item aberto e escrito é melhor que fechá-la com um item aberto
e esquecido.

**Total:** ~10 a 13 semanas em ritmo de estudante com outras matérias. As fases 0–3 já entregam um projeto respeitável e publicável; 4–6 são o que o transformam em peça de portfólio.

---

## 12. Armadilhas conhecidas

| Armadilha | Como evitar |
|---|---|
| Views `TypedArray` desanexando quando a heap cresce | recriar a view após **toda** chamada ao WASM (§4.2) |
| Endereço de `malloc` reciclado virando id de nó | `idmap` com `id_esquece()` no `free` (§3.5) |
| Enums dessincronizados entre C e TS | gerar o `.ts` a partir do `.h` (§5.1) |
| Layout de árvore ingênuo (`x = pai ± w/2^nivel`) | Reingold–Tilford desde a primeira árvore |
| Animação dentro do estado do React | `Player` + canvas fora do React; React só nos painéis, throttled |
| Trace estourando em quicksort de n grande | `TRACE_CAP` + flag de truncado + limite de n no modo animado |
| Recursão profunda estourando a stack do WASM | `-sSTACK_SIZE=1MB` e limitar n |
| Implementar "desfazer" evento por evento | reexecutar de 0 até k (§1.5) |
| Escopo: querer a ementa inteira antes de publicar | publicar no fim da Fase 3 e ir crescendo |
| Escrever código de desenho em C | o C **nunca** desenha; só emite eventos |

---

## 13. O que faz isto virar portfólio (e não só um trabalho)

Em ordem de retorno sobre esforço:

1. **README com GIFs** logo no topo, link do demo ao lado. Recrutador não clona repositório.
2. **Uma seção "decisões de arquitetura"** explicando por que trace de eventos e por que zero JSON no C. Mostra que você pensa em design, não só em sintaxe.
3. **CI verde com ASAN** — em um projeto C, isso diz "eu sei o que é gerenciamento de memória".
4. **Fuzz diferencial** — quase ninguém em nível de graduação faz.
5. **O modo empírico** (§9.4) — teoria confrontada com medição.
6. **Deploy funcionando** — um link que abre e funciona vale mais que 5 000 linhas.
7. Commits em português ou inglês, mas **consistentes**, e mensagens que expliquem o porquê.

---

## 14. Decisões fechadas

- [x] **Estruturas.** O quadro da §8.2, com pilha e fila nas duas implementações (§8.3).
- [x] **Tipo dos dados: só `int`.** Nada de `void*` com comparador. Ganhos concretos: `ds_call(op, a, b, c)` continua sendo quatro inteiros, o buffer de eventos continua sendo `Int32Array`, e nenhuma estrutura precisa de ponteiro de função para comparar. Se um dia quiser genérico, o caminho é `typedef int elem_t;` num header e trocar num lugar só — deixe esse `typedef` desde o começo, custa uma linha.
- [x] **Interface em pt/en**, com o núcleo agnóstico de idioma. Detalhes na §7.6 — e é decisão de Fase 0, não de Fase 6.
- [x] **Sem modo quiz.** Fora do escopo, em qualquer fase.
- [x] **Nome e URL.** `simux`, e `atrudev.github.io/simux` pelo GitHub
      Pages — de graça, com HTTPS, e sem prender nada: um domínio próprio
      pluga depois com um `CNAME`, e renomear o repositório mantém os
      links antigos redirecionando.

### 14.1 Nome e URL

**Nome: `simux`.** Aplicado em todo o canvas de design e neste documento. Wordmark em caixa baixa, para casar com a Space Grotesk.

Sobre a URL, que continua em aberto e não é bloqueante:

- **Domínio próprio é opcional.** O GitHub Pages entrega `[SEU-USUARIO].github.io/simux` de graça, com HTTPS. Para portfólio isso basta — recrutador clica no link do README, não digita domínio. Um `.dev` ou `.com.br` custa uns R$ 40–60/ano e pluga depois com um arquivo `CNAME`, sem mexer em mais nada.
- **Renomear repositório no GitHub mantém os links antigos redirecionando**, então nada aqui te prende.
