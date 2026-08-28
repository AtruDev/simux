cmake_minimum_required(VERSION 3.20)

project(simux
    VERSION     0.1.0
    DESCRIPTION "Simulador visual de estruturas de dados"
    LANGUAGES   C)

set(CMAKE_C_STANDARD          99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS        OFF)   # C99 puro, sem extensões de compilador

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

option(DS_SANITIZE "Compila com AddressSanitizer e UndefinedBehaviorSanitizer" OFF)

# ---------------------------------------------------------------------------
# Opções herdadas por todo alvo do projeto.
# ---------------------------------------------------------------------------
add_library(ds_opcoes INTERFACE)

if(MSVC)
    target_compile_options(ds_opcoes INTERFACE /W4 /WX)
else()
    target_compile_options(ds_opcoes INTERFACE
        -Wall -Wextra -Werror
        -Wshadow                # variável de bloco escondendo outra
        -Wstrict-prototypes     # f() não é f(void) em C99
        -Wmissing-prototypes    # função pública sem declaração no header
        -Wpointer-arith
        -Wwrite-strings)
endif()

# ---------------------------------------------------------------------------
# Sanitizers.
#
# O alvo MinGW não tem runtime de sanitizer: o GCC aceita a flag e o link
# quebra depois, com um erro que não explica nada. Testamos antes e falhamos
# com uma mensagem útil. No CI, ASAN/UBSAN rodam no job Linux.
# ---------------------------------------------------------------------------
if(DS_SANITIZE)
    if(EMSCRIPTEN)
        message(FATAL_ERROR "DS_SANITIZE não se aplica ao build wasm.")
    endif()

    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS        "-fsanitize=address,undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address,undefined")
    check_c_source_compiles("int main(void) { return 0; }" DS_SANITIZER_DISPONIVEL)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)

    if(NOT DS_SANITIZER_DISPONIVEL)
        message(FATAL_ERROR
            "DS_SANITIZE=ON, mas este compilador não consegue linkar "
            "-fsanitize=address,undefined.\n"
            "O GCC do MinGW-w64 não distribui libasan/libubsan — sanitizer "
            "no alvo mingw não é suportado.\n"
            "Configure sem -DDS_SANITIZE=ON para o build local no Windows; "
            "o CI cobre ASAN/UBSAN no Linux.")
    endif()

    target_compile_options(ds_opcoes INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(ds_opcoes INTERFACE
        -fsanitize=address,undefined)
endif()

# ---------------------------------------------------------------------------
# web/src/core/ops.ts — gerado a partir dos headers de enums.
#
# Os mesmos enums existem nos dois lados da fronteira; mantidos na mão, eles
# dessincronizam, e o sintoma é mudo. O .h é a fonte, o .ts é derivado, e o
# .ts está no .gitignore para ninguém ser tentado a editá-lo.
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    set(DS_OPS_TS  ${CMAKE_CURRENT_SOURCE_DIR}/web/src/core/ops.ts)
    set(DS_GERADOR ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_enums.py)
    set(DS_HEADERS_ENUM
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include/ds/ids.h
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include/ds/erros.h)

    # O gerador não reescreve o arquivo quando o conteúdo não muda, para não
    # disparar rebuild do Vite à toa — daí o carimbo servir de OUTPUT.
    add_custom_command(
        OUTPUT     ${CMAKE_CURRENT_BINARY_DIR}/ops_ts.stamp
        BYPRODUCTS ${DS_OPS_TS}
        COMMAND    ${Python3_EXECUTABLE} ${DS_GERADOR} ${DS_HEADERS_ENUM}
                   --saida ${DS_OPS_TS}
        COMMAND    ${CMAKE_COMMAND} -E touch
                   ${CMAKE_CURRENT_BINARY_DIR}/ops_ts.stamp
        DEPENDS    ${DS_HEADERS_ENUM} ${DS_GERADOR}
        COMMENT    "gen_enums -> web/src/core/ops.ts"
        VERBATIM)

    add_custom_target(ops_ts ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ops_ts.stamp)
else()
    message(WARNING
        "Python não encontrado: web/src/core/ops.ts não será regerado. "
        "O build em C continua, mas o frontend pode ficar com enums velhos.")
endif()

# ---------------------------------------------------------------------------
# core — C99, zero dependências. Compila igual nativo e para wasm.
#
# api.c fica de fora de propósito: é o único arquivo que conhece o Emscripten,
# e entra direto em cada executável, para os símbolos exportados não serem
# descartados por vir de dentro de um .a.
# ---------------------------------------------------------------------------
add_library(core STATIC
    core/trace/trace.c
    core/trace/idmap.c
    core/ds/pilha_enc.c
    core/ds/pilha_vet.c
    core/ds/fila_enc.c
    core/ds/fila_vet.c
    core/ds/lista_simples.c
    core/ds/lista_dupla.c
    core/ds/lista_circular.c
    core/arvore/abb.c
    core/arvore/avl.c
    core/busca/vetor_ord.c
    core/disco/paginador.c
    core/disco/arvore_b.c
    core/disco/arvore_bmais.c
    core/hash/hash_enc.c
    core/hash/hash_abe.c
    core/sort/medida.c
    core/sort/ordenacao.c
    core/sort/cena.c
    core/sort/bolha.c
    core/sort/selecao.c
    core/sort/insercao.c
    core/sort/shell.c
    core/sort/quick.c
    core/sort/merge.c
    core/util/aleatorio.c)

target_include_directories(core PUBLIC core/include)
target_include_directories(core PRIVATE core/ds core/sort core/busca core/arvore core/hash core/disco)
target_link_libraries(core PUBLIC ds_opcoes)

# ---------------------------------------------------------------------------
# Build wasm: um módulo ES em web/src/wasm/, consumido pelo Vite.
#
# api.c entra direto no executável, e não via libcore.a, para os símbolos
# marcados com EMSCRIPTEN_KEEPALIVE não dependerem de já estarem referenciados
# para serem puxados do arquivo.
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    add_executable(simux core/api/api.c)
    target_link_libraries(simux PRIVATE core)

    set_target_properties(simux PROPERTIES
        SUFFIX                   ".js"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web/src/wasm)

    target_link_options(simux PRIVATE
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME=criarSimux
        -sENVIRONMENT=web

        # Crescer a heap desanexa as views antigas; o bridge recria a view
        # depois de toda chamada, o que torna isto seguro.
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16MB

        # Quicksort e mergesort recursivos precisam de pilha.
        -sSTACK_SIZE=1MB

        # UTF8ToString ficou de fora de propósito: nenhuma string atravessa a
        # fronteira, e exportá-la só convidaria alguém a mandar texto do C.
        -sEXPORTED_RUNTIME_METHODS=HEAP32,HEAPU8
        -sEXPORTED_FUNCTIONS=_ds_sessao_nova,_ds_sessao_fim,_ds_sessao_slot,_ds_sessao_slots,_ds_tipo_sessao,_ds_tamanho,_ds_capacidade,_ds_call,_ds_erro,_ds_trace_ptr,_ds_trace_len,_ds_trace_truncado,_ds_buffer,_ds_bench,_ds_bench_escritas,_malloc,_free)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(simux PRIVATE -flto)
        target_link_options(simux PRIVATE -flto --closure 1)
    else()
        target_link_options(simux PRIVATE -sASSERTIONS=2 -sSAFE_HEAP=1)
    endif()
endif()

# ---------------------------------------------------------------------------
# Alvos nativos: CLI e testes. O build wasm não os constrói.
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    add_executable(cli cli/main.c core/api/api.c)
    target_link_libraries(cli PRIVATE core)
    target_compile_definitions(cli PRIVATE SIMUX_VERSAO="${PROJECT_VERSION}")

    enable_testing()

    add_executable(testes
        core/api/api.c
        tests/runner.c
        tests/test_trace.c
        tests/test_idmap.c
        tests/test_pilha.c
        tests/test_fila.c
        tests/test_fuzz.c
        tests/test_lista.c
        tests/test_ordenacao.c
        tests/test_busca.c
        tests/test_arvore.c
        tests/test_hash.c
        tests/test_arvore_b.c
        tests/test_arvore_bmais.c
        tests/test_api.c)
    target_include_directories(testes PRIVATE tests core/ds core/sort core/busca core/arvore core/hash core/disco)
    target_link_libraries(testes PRIVATE core)

    add_test(NAME nucleo COMMAND testes)
endif()
