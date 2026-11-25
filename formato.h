#ifndef FORMATO_H
#define FORMATO_H

#include <stddef.h>

int formato_validar_codigo(const char *codigo);

int formato_respuesta(const char *codigo, char *buffer, size_t tam);

#endif