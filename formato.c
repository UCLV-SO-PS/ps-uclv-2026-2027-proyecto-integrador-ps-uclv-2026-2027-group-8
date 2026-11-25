#include "formato.h"
#include "bd.h"
#include <stdio.h>
#include <string.h>

int formato_validar_codigo(const char *codigo) {
    if (!codigo || strlen(codigo) != CODIGO_LEN) return 0;
    if (codigo[0] != 'S' || codigo[3] != 'E') return 0;
    for (int i = 1; i <= 2; i++) if (codigo[i] < '0' || codigo[i] > '9') return 0;
    for (int i = 4; i <= 5; i++) if (codigo[i] < '0' || codigo[i] > '9') return 0;
    return 1;
}

int formato_respuesta(const char *codigo, char *buffer, size_t tam) {
    if (!formato_validar_codigo(codigo)) {
        return snprintf(buffer, tam, "ERROR: codigo invalido\n");
    }
    const Episodio *e = bd_buscar(codigo);
    if (!e) {
        return snprintf(buffer, tam, "ERROR: episodio no encontrado\n");
    }
    return snprintf(buffer, tam, "%s | %s | %.1f | %ld votos\n",
                    e->codigo, e->titulo, e->puntuacion, e->votos);
}