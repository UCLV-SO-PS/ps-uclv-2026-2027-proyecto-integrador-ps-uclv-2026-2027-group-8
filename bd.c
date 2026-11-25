#include "bd.h"
#include <string.h>

static Episodio tabla[] = {
    {"S01E01", "The Hedge Knight",           9.2, 45120},
    {"S01E02", "Hard Salt Beef",             8.8, 38900},
    {"S01E03", "The Squire",                 9.5, 52400},
    {"S01E04", "Seven",                      9.1, 41200},
    {"S01E05", "In the Name of the Mother",  9.7, 68300},
    {"S01E06", "The Morrow",                 9.4, 49800}
};

static const int N = (int)(sizeof(tabla)/sizeof(tabla[0]));

void bd_init(void) { 
}

const Episodio *bd_buscar(const char *codigo) {
    for (int i = 0; i < N; i++) {
        if (strcmp(tabla[i].codigo, codigo) == 0) {
            return &tabla[i];
        }
    }
    return NULL;
}

int bd_cantidad(void) { 
    return N; 
}