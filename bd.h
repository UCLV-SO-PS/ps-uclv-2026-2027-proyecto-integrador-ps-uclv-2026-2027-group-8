#ifndef BD_H
#define BD_H

#include <stddef.h>

#define CODIGO_LEN 6
#define TITULO_MAX 128

typedef struct {
    char   codigo[CODIGO_LEN + 1]; 
    char   titulo[TITULO_MAX];
    double puntuacion;
    long   votos;
} Episodio;

void bd_init(void);

const Episodio *bd_buscar(const char *codigo);

int bd_cantidad(void);

#endif