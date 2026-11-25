#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bd.h"
#include "formato.h"

int main(int argc, char **argv) {
    int test_run = (argc > 1 && strcmp(argv[1], "--test-run") == 0);

    bd_init();
    printf("bd cargada: %d episodios\n", bd_cantidad());

    if (test_run) {
        printf("--test-run OK\n");
        return 0;
    }

    return 0;
}