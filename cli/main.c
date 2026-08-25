/* cli/main.c — binário de terminal sobre o mesmo core que vai para o wasm.
 *
 * Por enquanto só confirma que o core linka. Cresce junto com as estruturas:
 * é ele que serve de ferramenta de depuração e dos trabalhos da matéria. */

#include <stdio.h>

int main(void)
{
    printf("simux %s\n", SIMUX_VERSAO);
    return 0;
}
