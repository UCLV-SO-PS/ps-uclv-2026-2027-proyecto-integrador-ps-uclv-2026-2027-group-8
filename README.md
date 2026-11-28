# Proyecto Integrador — Servidor Concurrente IMDb AKotSK

Servidor concurrente TCP en C que permite consultar, en tiempo real, la puntuación IMDb y los votos de los episodios de la serie **"A Knight of the Seven Kingdoms"**. Proyecto final de la asignatura **Programación de Sistemas** (curso 2026–2027).

## Equipo
- Nombre: Vividor Lovers
- Integrantes: Jorge Daniel Vidaurreta Alvarez, Grasiel Aguilar Morejon, Cesar Yunior Martin Perez

## Estructura del proyecto

| Archivo | Descripción |
|---|---|
| `Makefile` | Reglas `all` y `clean`. |
| `bd.c` / `bd.h` | Base de datos de episodios (`bd_init`, `bd_buscar`, `bd_cantidad`). |
| `formato.c` / `formato.h` | Validación del código y formateo de la respuesta. |
| `libmonitor.a` | Biblioteca estática que empaqueta `bd.o` y `formato.o`. |
| `server.c` / `server` | Servidor concurrente TCP. |
| `client.c` / `client` | Cliente de consola para probar el servidor. |
| `screenshot/` | Capturas de pantalla del funcionamiento. |

## Requisitos
- Linux (o WSL en Windows) con `gcc`, `make` y `ar`.
- El puerto TCP **6666** debe estar libre.

## Compilación
```bash
make all     # compila, empaqueta libmonitor.a (ar rcs) y genera server y client
make clean   # elimina *.o, libmonitor.a, server y client
```

## Ejecución

**1. Prueba automática (smoke test):**
```bash
./server --test-run
echo $?      # debe imprimir 0
```
Inicializa la base de datos, crea el socket, lo enlaza al puerto 6666 y lo pone a escuchar. Si todo va bien imprime `--test-run OK` y termina con código 0, sin quedarse esperando conexiones.

**2. Servidor y cliente:**
```bash
# Terminal 1
./server

# Terminal 2
./client
```
El cliente pide el código del episodio por consola, lo envía al servidor y muestra la respuesta.

**3. Detener el servidor:**
El servidor ignora `CTRL+C` (SIGINT) por requisito del proyecto, así que se detiene desde otra terminal:
```bash
pkill server
```

## Ejemplo de uso

Cliente:
```
$ ./client
Codigo del episodio (ej. S01E01): S01E01
S01E01 | The Hedge Knight | 9.2 | 45120 votos
```

Código que no existe:
```
$ ./client
Codigo del episodio (ej. S01E01): S09E09
ERROR: episodio no encontrado
```

Código con longitud incorrecta (el cliente lo rechaza antes de enviarlo):
```
$ ./client
Codigo del episodio (ej. S01E01): S1E1
Error: el codigo debe tener exactamente 6 caracteres
```

Salida del servidor (una línea por cada conexión atendida):
```
bd cargada: 6 episodios
Servidor escuchando en el puerto 6666...
Conexion atendida por el hijo PID 97 - IP: 127.0.0.1
```

Las capturas de pantalla de estas ejecuciones están en la carpeta [`screenshot/`](screenshot/).

## Protocolo cliente–servidor
1. El cliente se conecta a `127.0.0.1:6666`.
2. Envía exactamente **6 bytes** con el código del episodio (por ejemplo `S01E01`), sin `\n` ni `\0`.
3. El servidor responde con una cadena de texto ya formateada y cierra la conexión.
4. El cliente lee la respuesta hasta que el servidor cierra y la imprime por pantalla.

## Decisiones de diseño

### Concurrencia
- El proceso padre escucha en el puerto 6666 y por cada `accept()` crea un proceso hijo con `fork()`. Así, un cliente lento o que se desconecta no bloquea a los demás.
- El hijo cierra el socket de escucha (`server_fd`) y el padre cierra el socket del cliente (`client_fd`), para que cada proceso conserve solo los descriptores que necesita.
- Se usa `SO_REUSEADDR` para poder reiniciar el servidor sin esperar a que el puerto salga del estado `TIME_WAIT`. La cola de espera de `listen()` es de 10 conexiones.

### Comunicación interproceso (pipe)
- Por cada cliente se crea una tubería sin nombre con `pipe()` antes del `fork()`.
- El hijo escribe en ella la IP del cliente (obtenida con `inet_ntop`) **antes** de atender la solicitud, y el padre la lee e imprime junto con el PID del hijo: `Conexion atendida por el hijo PID <pid> - IP: <ip>`.
- La IP viaja como cadena terminada en `'\0'`, de modo que el padre sabe dónde termina sin necesidad de enviar su longitud. Cada extremo de la tubería que no se usa se cierra en cada proceso.

### Señales y control de procesos
- **SIGINT:** se ignora con `sigaction()` y `SIG_IGN`.
- **SIGCHLD:** el padre instala un manejador con `sigaction()` que ejecuta `waitpid(-1, NULL, WNOHANG)` en bucle, recogiendo todos los hijos terminados y evitando procesos zombi. Se usa `SA_RESTART` y se guarda/restaura `errno` dentro del manejador.
- El hijo restaura `SIGCHLD` a su comportamiento por defecto, ya que no necesita el manejador del padre.

### Entrada/salida robusta sobre TCP
- TCP es un flujo de bytes y no garantiza que un `read`/`send` transfiera todo de una vez. Por eso el servidor usa funciones auxiliares (`leer_exactamente`, `enviar_completo`, `escribir_pipe`) que repiten la operación hasta completar los bytes pedidos y reintentan si la llamada es interrumpida por una señal (`EINTR`).
- Se envía con `MSG_NOSIGNAL` para que un cliente que cierra antes de tiempo no mate al hijo con `SIGPIPE`.
- El cliente también lee con `recv()` en bucle hasta que el servidor cierra la conexión, y añade el `'\0'` final porque el servidor no lo envía.

### Biblioteca estática `libmonitor.a`
- La base de datos (`bd.c`) y las utilidades de validación y formato (`formato.c`) se compilan a objetos y se empaquetan con `ar rcs`, separando la lógica de datos de la lógica de red (`server.c`).
- El servidor se enlaza con `-L. -lmonitor`.
- El Makefile declara las dependencias de cada objeto, así que `make` solo recompila lo que cambió.

### Validación
- El cliente comprueba que el código tenga exactamente 6 caracteres (quitando el salto de línea que deja `fgets`) antes de enviarlo.
- El servidor valida y busca el código en la base de datos; si no existe, responde con `ERROR: episodio no encontrado`.

### Smoke test (`--test-run`)
- Ejecuta todo el arranque real del servidor (base de datos, socket, `bind`, `listen` y señales) y sale con código 0 antes de entrar al bucle de `accept()`. Esto permite verificar el despliegue de forma automática, por ejemplo desde el workflow de autocorrección de GitHub.

## Base de datos de episodios

| Código | Episodio | Puntuación IMDb | Votos |
|---|---|---|---|
| S01E01 | The Hedge Knight | 9.2 | 45,120 |
| S01E02 | Hard Salt Beef | 8.8 | 38,900 |
| S01E03 | The Squire | 9.5 | 52,400 |
| S01E04 | Seven | 9.1 | 41,200 |
| S01E05 | In the Name of the Mother | 9.7 | 68,300 |
| S01E06 | The Morrow | 9.4 | 49,800 |
